//
// Created by Jamie Cotter on 01/07/2025.
//

#include "HeavySchedulingFunctions.h"
#include "../../model/data_models/TimeSourceSingleton/TimeSourceSingleton.h"
#include "../../Services/NetworkServices/NetworkServices.h"
#include "../../Constants/RequestSizes.h"
#include "../LowCompServices/LowCompServices.h"
#include "../../model/data_models/WorkItems/ProcessingItem/LowProcessingItem/LowProcessingItem.h"
#include "../../utils/UtilFunctions/UtilFunctions.h"
#include "../../Constants/FTP_CONFIG.h"
#include "../../Constants/AllocationMacros.h"
#include "../HighCompServices/HighCompServices.h"
#include "../../model/data_models/WorkItems/HaltWorkItem/HaltWorkItem.h"

namespace services {

    void state_update_function(std::map<std::string, std::shared_ptr<model::BaseCompResult>> &total_offload_list,
                               std::map<std::string, std::shared_ptr<model::LowCompResult>> &total_off_low,
                               std::map<std::string, std::shared_ptr<model::HighCompResult>> &total_off_high,
                               std::string dnn_id,
                               std::shared_ptr<model::BaseCompResult> dnn,
                               std::shared_ptr<model::ComputationDevice> device,
                               std::vector<std::shared_ptr<model::NetCommunicationBase>> &network_link,
                               bool isSuccess,
                               std::chrono::time_point<std::chrono::system_clock> finish_time
    ) {

        device->DNNS.erase(std::remove_if(device->DNNS.begin(), device->DNNS.end(),
                                          [dnn_id](std::shared_ptr<model::BaseCompResult> br) {
                                              return dnn_id == br->getDnnId();
                                          }), device->DNNS.end());


        /* Begin iterating through the DNN to gather tasks and links to remove */
        std::vector<std::shared_ptr<model::NetCommunicationBase>> links_to_prune;

        if (dnn->getDnnType() != enums::dnn_type::low_comp && dnn->getSrcHost() != dnn->getAllocatedHost())
            links_to_prune.push_back(std::static_pointer_cast<model::HighCompResult>(dnn)->task_allocation);

        for (const auto &prune_link: links_to_prune) {
            auto link_id = prune_link->link_activity_id;
            network_link.erase(std::remove_if(network_link.begin(),
                                              network_link.end(),
                                              [link_id](
                                                      std::shared_ptr<model::NetCommunicationBase> linkAct) {
                                                  return linkAct->link_activity_id == link_id;
                                              }), network_link.end());
        }

        total_offload_list.erase(dnn->getDnnId());

        if (dnn->getDnnType() != enums::dnn_type::low_comp)
            total_off_high.erase(dnn->getDnnId());
        else
            total_off_low.erase(dnn->getDnnId());


        if (isSuccess)
            dnn->setActualFinish(finish_time);

    }

    std::pair<std::shared_ptr<model::LowCompResult>, std::shared_ptr<model::TimeWindow>>
    low_comp_allocation_func(uint64_t bw_bytes,
                             std::shared_ptr<model::Network> network,
                             std::string host, std::shared_ptr<model::WorkItem> item) {
        auto proc_item = std::static_pointer_cast<model::LowProcessingItem>(item);

        /*Need to create a copy of the network list so that we can keep track of incomplete net allocations */
        std::vector<std::shared_ptr<model::NetCommunicationBase>> copyList;
        for (const auto &i: network->network_link) {
            copyList.push_back(i);
        }

        std::chrono::time_point<std::chrono::system_clock> currentTime = model::TimeSourceSingleton::getTime();

        auto times = services::findLinkSlot(
                currentTime, float(bw_bytes), LOW_TASK_SIZE, copyList);


        std::sort(copyList.begin(), copyList.end(),
                  [](const std::shared_ptr<model::NetCommunicationBase> &a,
                     const std::shared_ptr<model::NetCommunicationBase> &b) {
                      return a->timeWindow->stop < b->timeWindow->stop;
                  });

        /* Using the input upload window for each task generated we now allocate a time slot on their respective hosts */
        auto result = services::allocate_task(item, network->devices, times);
        auto time_window = result.second;

        /* If we cannot allocate a device for even one task we instead create a new allocation request and a halt request */
        if (!result.first) {

            return std::make_pair(nullptr, time_window);

        } else {
            /* For each allocated low comp task we create a Result object
             * the key is the dnn_id*/

            auto deadline = proc_item->getDeadline();
            auto bR = std::make_shared<model::LowCompResult>(proc_item->getDnnIdAndDevice().first, host, 1, deadline,
                                                             time_window->start,
                                                             time_window->stop,
                                                             enums::dnn_type::low_comp
            );

            auto state_times = services::findLinkSlot(
                    time_window->stop + std::chrono::milliseconds{1}, float(bw_bytes), STATE_UPDATE_SIZE, copyList);

            return std::make_pair(bR, time_window);
        }
    }

    std::vector<std::shared_ptr<model::HighCompResult>>
    high_comp_allocation_func(bool isReallocation, std::shared_ptr<model::HighProcessingItem> processingItem,
                              std::map<std::string, std::shared_ptr<model::BaseCompResult>> &off_total,
                              std::map<std::string, std::shared_ptr<model::ComputationDevice>> copyDeviceWorkloadList,
                              std::vector<std::shared_ptr<model::NetCommunicationBase>> copyList,
                              uint64_t bw_bytes,
                              std::string sourceHost) {

        std::vector<std::shared_ptr<model::HighCompResult>> resultDNNs;

        std::map<std::string, std::shared_ptr<model::HighCompResult>> baseResult;
        auto currentTime = model::TimeSourceSingleton::getTime();

        if (isReallocation) {
            baseResult = processingItem->baseResult;
            for (const auto &[dnn_id, baseRes]: baseResult) {
                baseRes->setVersion(model::TimeSourceSingleton::getTime().time_since_epoch().count() * 1000);
            }
        }

        std::vector<std::chrono::time_point<std::chrono::system_clock>> finish_times;

        for (const auto &[dnn_id, dnn_task]: off_total) {
            auto finish = dnn_task->estimated_start_fin->stop;

#if SOFT_DEADLINES == 0
            if (finish < processingItem->getDeadline() && finish > currentTime)
#else
                if (finish > model::TimeSourceSingleton::getTime())
#endif
                finish_times.push_back(finish);
        }

        auto now = currentTime;
        finish_times.push_back(now);
        std::sort(finish_times.begin(), finish_times.end());

        std::vector<std::string> allocated_tasks;

        for (const auto &finish_time: finish_times) {
            auto allocationMap = utils::generateAllocationMap(copyDeviceWorkloadList);

            std::vector<std::shared_ptr<model::HighCompResult>> results;

            for (const auto &dnn_id: processingItem->getDnnIds()) {
                if (utils::is_allocated(dnn_id, allocated_tasks))
                    continue;

                //This window is only used if the task is allocated to a device other than its source
                auto data_transfer_time_base = finish_time + std::chrono::milliseconds{10};
                auto data_transfer_window = services::findLinkSlot(data_transfer_time_base, bw_bytes,
                                                                   TASK_FORWARD_SIZE,
                                                                   copyList);

                std::shared_ptr<model::LinkAct> data_transfer_link_act = std::make_shared<model::LinkAct>();
                data_transfer_link_act->setIsMeta(false);
                data_transfer_link_act->setDataSize(TASK_FORWARD_SIZE);
                data_transfer_link_act->timeWindow = std::make_shared<model::TimeWindow>(
                        data_transfer_window->start, data_transfer_window->stop);

                auto src_start_time = currentTime + std::chrono::milliseconds{10};
                auto remote_start_time =
                        data_transfer_link_act->timeWindow->stop + std::chrono::milliseconds{1};

                int current_core = FTP_LOW_CORE;

                int CURRENT_N = FTP_LOW_N;
                int CURRENT_M = FTP_LOW_M;

                std::chrono::time_point<std::chrono::system_clock> finish_time_on_src =
                        src_start_time +
                        std::chrono::milliseconds{
                                FTP_LOW_TIME};

                std::chrono::time_point<std::chrono::system_clock> finish_time_on_remote =
                        remote_start_time +
                        std::chrono::milliseconds{
                                FTP_LOW_TIME};

                auto selected_device = services::findNode(copyDeviceWorkloadList,
                                                          src_start_time,
                                                          finish_time_on_src,
                                                          data_transfer_link_act->timeWindow->stop,
                                                          finish_time_on_remote, sourceHost, current_core,
                                                          allocationMap, dnn_id);

                if (selected_device.first != TASK_NOT_FOUND) {
                    std::shared_ptr<model::HighCompResult> task = std::make_shared<model::HighCompResult>(dnn_id,
                                                                                                          sourceHost,
                                                                                                          processingItem->getDeadline(),
                                                                                                          enums::dnn_type::high_comp);
                    if (selected_device.second != sourceHost) {

                        task->task_allocation = (data_transfer_link_act);
                        task->estimated_start_fin = std::make_shared<model::TimeWindow>(remote_start_time,
                                                                                        finish_time_on_remote);

                    } else {
                        task->estimated_start_fin = std::make_shared<model::TimeWindow>(
                                src_start_time, finish_time_on_src);
                    }

                    task->setAllocatedHost(selected_device.second);
                    task->setCoreAllocation(current_core);
                    task->setN(CURRENT_N);
                    task->setM(CURRENT_M);

                    copyDeviceWorkloadList[selected_device.second]->DNNS.push_back(task);
                    results.push_back(task);
                    allocationMap[selected_device.second] += 1;
                }
            }

            for (const auto &task: results) {

                std::chrono::time_point<std::chrono::system_clock> finish_time_on_device =
                        task->estimated_start_fin->start +
                        std::chrono::milliseconds{
                                FTP_HIGH_TIME};
                auto core_usage = calculateDeviceCoreUsage(task->estimated_start_fin->start, finish_time_on_device,
                                                           copyDeviceWorkloadList[task->getAllocatedHost()],
                                                           task->getDnnId());


                if (core_usage + FTP_HIGH_CORE <=
                    copyDeviceWorkloadList[task->getAllocatedHost()]->getCores()) {
                    task->setN(FTP_HIGH_N);
                    task->setM(FTP_HIGH_M);
                    task->setCoreAllocation(FTP_HIGH_CORE);
                    task->estimated_start_fin->stop = finish_time_on_device;
                }

                /* If the current allocation does not satisfy deadline,
                 * remove link tasks from link and task from allocated host
                 * so that we can continue to attempt to allocate */
                if (task->estimated_start_fin->stop > processingItem->getDeadline()) {
                    if (task->getAllocatedHost() != task->getSrcHost()) {
                        int comm_id = task->task_allocation->link_activity_id;

                        copyList.erase(
                                std::remove_if(copyList.begin(),
                                               copyList.end(),
                                               [comm_id](std::shared_ptr<model::NetCommunicationBase> la) {
                                                   return comm_id == la->link_activity_id;
                                               }), copyList.end());

                    }

                    auto dnn_id = task->getDnnId();
                    auto allocated_device = task->getAllocatedHost();
                    copyDeviceWorkloadList[allocated_device]->DNNS.erase(
                            std::remove_if(copyDeviceWorkloadList[allocated_device]->DNNS.begin(),
                                           copyDeviceWorkloadList[allocated_device]->DNNS.end(),
                                           [dnn_id](std::shared_ptr<model::BaseCompResult> br) {
                                               return dnn_id == br->getDnnId();
                                           }), copyDeviceWorkloadList[allocated_device]->DNNS.end());

                } else {
                    allocated_tasks.push_back(task->getDnnId());
                    resultDNNs.push_back(task);
                }
            }

            if (allocated_tasks.size() == processingItem->getDnnIds().size())
                break;
        }

        return resultDNNs;
    }

    std::tuple<std::shared_ptr<model::BaseCompResult>,
            std::vector<std::shared_ptr<model::BaseCompResult>>,
			std::vector<std::shared_ptr<model::NetCommunicationBase>>,
            std::map<std::string, std::shared_ptr<model::BaseCompResult>>,
            std::map<std::string, std::shared_ptr<model::HighCompResult>>, uint64_t>
    halt_call_func(std::shared_ptr<model::HaltWorkItem> haltWorkItem,
                   std::vector<std::shared_ptr<model::BaseCompResult>> deviceTaskLoad,
                   std::vector<std::shared_ptr<model::NetCommunicationBase>> copyList,
                   std::map<std::string, std::shared_ptr<model::BaseCompResult>> off_total,
                   std::map<std::string, std::shared_ptr<model::HighCompResult>> off_high) {
        std::vector<std::string> hostList;

        auto sourceDeviceId = haltWorkItem->getHostToExamine();
        auto startTime = haltWorkItem->getStartTime();
        auto finTime = haltWorkItem->getFinTime();

        std::vector<std::pair<int, std::shared_ptr<model::HighCompResult>>> haltCandidates;

        for (int i = 0; i < deviceTaskLoad.size(); i++) {
            if (startTime <= deviceTaskLoad[i]->estimated_start_fin->stop && deviceTaskLoad[i]->estimated_start_fin->start <= finTime &&
                deviceTaskLoad[i]->getDnnType() == enums::dnn_type::high_comp)
                haltCandidates.emplace_back(i, std::static_pointer_cast<model::HighCompResult>(deviceTaskLoad[i]));
        }

        std::shared_ptr<model::HighCompResult> dnnToPrune;

        for (int i = 0; i < haltCandidates.size(); i++) {
            if (i == 0) {
                dnnToPrune = haltCandidates[i].second;
            } else {
                if (dnnToPrune->getDeadline() < haltCandidates[i].second->getDeadline())
                    dnnToPrune = haltCandidates[i].second;
            }
        }

        if (dnnToPrune == nullptr) {
           return std::make_tuple(nullptr, deviceTaskLoad, copyList, off_total, off_high, 0);
        }

        auto dnn_id = dnnToPrune->getDnnId();
        auto allocated_device = dnnToPrune->getAllocatedHost();
        deviceTaskLoad.erase(
                std::remove_if(deviceTaskLoad.begin(),
                               deviceTaskLoad.end(),
                               [dnn_id](std::shared_ptr<model::BaseCompResult> br) {
                                   return dnn_id == br->getDnnId();
                               }), deviceTaskLoad.end());

        auto versionToPrune = dnnToPrune->getVersion();

        dnnToPrune->setCoreAllocation(0);
        dnnToPrune->estimated_start_fin = std::make_shared<model::TimeWindow>(
                std::chrono::time_point<std::chrono::system_clock>(std::chrono::milliseconds{0}),
                std::chrono::time_point<std::chrono::system_clock>(std::chrono::milliseconds{0}));
        dnnToPrune->setVersion(model::TimeSourceSingleton::getTime().time_since_epoch().count() * 1000);
        dnnToPrune->setN(0);
        dnnToPrune->setM(0);

        std::vector<std::shared_ptr<model::NetCommunicationBase>> links_to_prune;
        if (dnnToPrune->getSrcHost() == dnnToPrune->getAllocatedHost())
            links_to_prune.push_back(dnnToPrune->task_allocation);

        for (const auto &prune_link: links_to_prune) {
            auto link_id = prune_link->link_activity_id;
            copyList.erase(std::remove_if(copyList.begin(),
                                          copyList.end(),
                                          [link_id](
                                                  std::shared_ptr<model::NetCommunicationBase> linkAct) {
                                              return linkAct->link_activity_id == link_id;
                                          }), copyList.end());
        }

        dnnToPrune->setAllocatedHost("");

        off_total.erase(dnnToPrune->getDnnId());
        off_high.erase(dnnToPrune->getDnnId());

        /* We attempt to reallocate the DNN if possible */
        auto host_list = std::make_shared<std::vector<std::string>>(
                std::initializer_list<std::string>{dnnToPrune->getSrcHost()});

        return std::make_tuple(dnnToPrune, deviceTaskLoad, copyList, off_total, off_high, versionToPrune);
    }
} // services