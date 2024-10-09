//
// Created by jamiec on 9/23/22.
//

#include <thread>
#include <random>
#include <utility>
#include "WorkQueueManager.h"
#include "../../model/data_models/NetworkCommsModels/BandwidthTestCommsModel/BandwidthTestCommsModel.h"
#include "../../model/data_models/WorkItems/ProcessingItem/HighProcessingItem/HighProcessingItem.h"
#include "../LowCompServices/LowCompServices.h"
#include "../../model/data_models/WorkItems/StateUpdate/StateUpdate.h"
#include "../NetworkServices/NetworkServices.h"
#include "../../Constants/ModeMacros.h"
#include "../../utils/UtilFunctions/UtilFunctions.h"
#include "../../Constants/CLIENT_DETAILS.h"
#include "../HighCompServices/HighCompServices.h"
#include "../../Constants/AllocationMacros.h"
#include "../../model/data_models/NetworkCommsModels/HighComplexityAllocation/HighComplexityAllocationComms.h"
#include "../../model/data_models/WorkItems/ProcessingItem/LowProcessingItem/LowProcessingItem.h"
#include "../../model/data_models/NetworkCommsModels/LowComplexityAllocation/LowComplexityAllocationComms.h"
#include "../../model/data_models/WorkItems/PruneItem/PruneItem.h"
#include "../../model/data_models/NetworkCommsModels/HaltNetworkCommsModel/HaltNetworkCommsModel.h"
#include "../../model/data_models/WorkItems/HaltWorkItem/HaltWorkItem.h"
#include "../../Constants/RequestSizes.h"
#include "../../Constants/FTP_CONFIG.h"
#include "../../model/data_models/WorkItems/BandwidthUpdate/BandwidthUpdateItem.h"

namespace services {
    std::atomic<int> WorkQueueManager::thread_counter = 0;

    static void update_bw_vals(std::shared_ptr<model::WorkItem> workItem, WorkQueueManager *queueManager) {
        auto bandwidth_update = std::static_pointer_cast<model::BandwidthUpdateItem>(workItem);

        auto bits_per_second_vect = bandwidth_update->bits_per_second_vect;

        for (int i = 0; i < bits_per_second_vect.size(); i++) {
            auto old_bps = queueManager->getAverageBitsPerSecond();
            auto old_jitter = queueManager->getJitter();

            auto diff = bits_per_second_vect[i] - old_bps;
            auto increment = EMA_ALPHA * diff;
            auto mean = old_bps + increment;
            auto variance = (1 - EMA_ALPHA) * (old_jitter);

            queueManager->setAverageBitsPerSecond(mean);
            queueManager->setJitter(variance);
        }

        web::json::value log;
        log["new_bps"] = web::json::value::number(queueManager->getAverageBitsPerSecond());
        log["new_jitter"] = web::json::value::number(queueManager->getJitter());
        queueManager->logManager->add_log(enums::LogTypeEnum::BANDWIDTH_NEW_VALUE, log);

        queueManager->add_task(std::make_shared<model::WorkItem>(enums::request_type::network_disc));
        queueManager->decrementThreadCounter();
    }

    void update_network_disc(std::shared_ptr<model::WorkItem> workItem, WorkQueueManager *queueManager) {
        auto bytes_per_millisecond = queueManager->getBytesPerMillisecond();
        auto base_comm_size = static_cast<uint64_t>(TASK_FORWARD_SIZE / bytes_per_millisecond);

        std::chrono::time_point<std::chrono::system_clock> current_time_of_reasoning = std::chrono::system_clock::now();

        std::vector<std::shared_ptr<model::Bucket>> new_network_list = services::cascade_function(
                current_time_of_reasoning, BASE_BUCKET_COUNT, queueManager->network->network_link, base_comm_size,
                EXP_BUCKET_COUNT);

        queueManager->network->network_link = new_network_list;
        queueManager->network->last_time_of_reasoning = current_time_of_reasoning;

        web::json::value log;
        log["network"] = queueManager->network->convertToJson();

        queueManager->logManager->add_log(enums::LogTypeEnum::NEW_BUCKET_LINK, log);
        queueManager->decrementThreadCounter();
    }

    void state_update_call(std::shared_ptr<model::WorkItem> item, WorkQueueManager *queueManager) {
        std::shared_ptr<model::StateUpdate> stateUpdate = std::static_pointer_cast<model::StateUpdate>(item);

        std::unique_lock<std::mutex> off_lock(queueManager->offloaded_lock, std::defer_lock);
        off_lock.lock();


        auto dnn = queueManager->off_total[stateUpdate->getDnnId()];

        std::unique_lock<std::mutex> net_lock(queueManager->network_lock, std::defer_lock);

        net_lock.lock();

        std::string allocated_device = dnn->getAllocatedHost();

        auto dnn_id = dnn->getDnnId();
        queueManager->network->devices[allocated_device]->DNNS.erase(
                std::remove_if(queueManager->network->devices[allocated_device]->DNNS.begin(),
                               queueManager->network->devices[allocated_device]->DNNS.end(),
                               [dnn_id](std::shared_ptr<model::BaseCompResult> br) {
                                   return dnn_id == br->getDnnId();
                               }), queueManager->network->devices[allocated_device]->DNNS.end());


        queueManager->off_total.erase(dnn->getDnnId());

        if (dnn->getDnnType() != enums::dnn_type::low_comp)
            queueManager->off_high.erase(dnn->getDnnId());
        else
            queueManager->off_low.erase(dnn->getDnnId());


        net_lock.unlock();
        if (stateUpdate->isSuccess())
            dnn->setActualFinish(stateUpdate->getFinishTime());
        web::json::value log;

        if (dnn->getDnnType() == enums::dnn_type::low_comp) {
            log["dnn_details"] = std::static_pointer_cast<model::LowCompResult>(dnn)->convertToJson();
            queueManager->logManager->add_log(enums::LogTypeEnum::LOW_COMP_FINISH, log);
        } else {
            log["dnn_details"] = std::static_pointer_cast<model::HighCompResult>(dnn)->convertToJson();
            queueManager->logManager->add_log(stateUpdate->isSuccess() ? enums::LogTypeEnum::HIGH_COMP_FINISH
                                                                       : enums::LogTypeEnum::VIOLATED_DEADLINE, log);
        }


        off_lock.unlock();
        queueManager->decrementThreadCounter();
    }

    /*Function receives alow complexity DNN allocation request*/
    void low_comp_allocation_call(std::shared_ptr<model::WorkItem> item, WorkQueueManager *queueManager) {
        auto proc_item = std::static_pointer_cast<model::LowProcessingItem>(item);

        std::unique_lock<std::mutex> net_lock(queueManager->network_lock, std::defer_lock);
        net_lock.lock();
        std::map<std::string, std::shared_ptr<model::ComputationDevice>> &devices = queueManager->network->devices;

        net_lock.unlock();

        /* Fetching bytes and latency from the stored net parameters from iperf test */
        auto bw_bytes = static_cast<uint64_t>(queueManager->getBytesPerMillisecond());

        uint64_t data_size_bytes = queueManager->state_size;
        /* -------------------------------------------------------------------------- */

        std::chrono::time_point<std::chrono::system_clock> currentTime = std::chrono::system_clock::now() +
                                                                         std::chrono::milliseconds{
                                                                                 data_size_bytes /
                                                                                 bw_bytes} +
                                                                         std::chrono::milliseconds{
                                                                                 LOW_COMP_START_OFFSET_MS};

        auto estimated_fin = currentTime + std::chrono::milliseconds{LOW_COMPLEXITY_PROCESSING_TIME};


        auto [dnn_id, host] = proc_item->getDnnIdAndDevice();

        auto [index, window] = queueManager->network->devices[host]->resource_avail_windows[1]->containmentQuery(
                currentTime, estimated_fin);
        /* If we cannot allocate a device for even one task we instead create a new allocation request and a halt request */
        if (index == TASK_NOT_FOUND) {

            #if DEADLINE_PREMPT
            std::shared_ptr<model::WorkItem> workItem = std::make_shared<model::HaltWorkItem>(
                    enums::request_type::halt_req, host, currentTime,
                    estimated_fin);

            queueManager->add_task(workItem);
            auto newTask = std::make_shared<model::LowProcessingItem>(item->getHostList(),
                                                                      enums::request_type::low_complexity,
                                                                      proc_item->getDeadline(),
                                                                      proc_item->getDnnIdAndDevice());
            newTask->setReallocation(true);
            queueManager->add_task(std::static_pointer_cast<model::WorkItem>(newTask));

            web::json::value halt_log;
            halt_log["dnn_id"] = web::json::value::string(dnn_id);
            queueManager->logManager->add_log(enums::LogTypeEnum::HALT_REQUEST, halt_log);
            #endif

            web::json::value log;
            log["dnn_id"] = web::json::value::string(proc_item->getDnnIdAndDevice().first);
            log["current_time"] = web::json::value::number(
                    std::chrono::system_clock::now().time_since_epoch().count() * 1000);
            log["source_device"] = web::json::value::string(host);
            log["deadline"] = web::json::value::number(proc_item->getDeadline().time_since_epoch().count() * 1000);
            log["network"] = queueManager->network->convertToJson();
            queueManager->logManager->add_log(enums::LogTypeEnum::LOW_COMP_ALLOCATION_FAIL, log);
            queueManager->decrementThreadCounter();
            return;

        } else {
            /* For each allocated low comp task we create a Result object
             * the key is the dnn_id*/

            auto deadline = proc_item->getDeadline();
            auto bR = std::make_shared<model::LowCompResult>(dnn_id, host, 1, deadline, currentTime,
                                                             estimated_fin,
                                                             enums::dnn_type::low_comp
            );

            std::unique_lock<std::mutex> offload_lock(queueManager->offloaded_lock, std::defer_lock);
            offload_lock.lock();

            queueManager->off_low[bR->getDnnId()] = bR;
            queueManager->off_total[bR->getDnnId()] = bR;
            offload_lock.unlock();

            /* Add communication times to the network link */
            net_lock.lock();
            bR->setAllocatedHost(host);

            std::shared_ptr<model::LowComplexityAllocationComms> baseNetworkCommsModel =
                    std::make_shared<model::LowComplexityAllocationComms>(
                            enums::network_comms_types::low_complexity_allocation,
                            currentTime - std::chrono::milliseconds {data_size_bytes / bw_bytes},
                            bR,
                            host);

            queueManager->network->devices[bR->getSrcHost()]->DNNS.push_back(bR);

            web::json::value log;
            log["dnn_details"] = bR->convertToJson();
            queueManager->logManager->add_log(
                    proc_item->isReallocation() ? enums::LogTypeEnum::LOW_COMP_PREMPT_ALLOCATION_SUCCESS
                                                : enums::LogTypeEnum::LOW_COMP_ALLOCATION_SUCCESS, log);
            queueManager->networkQueueManager->addTask(baseNetworkCommsModel);

            auto tw = bR->estimated_start_fin;
            queueManager->network->devices[host]->resAvailRemoveAndSplit(tw, LOW_COMPLEXITY_CORE_COUNT);
            net_lock.unlock();
        }
        queueManager->decrementThreadCounter();
    }

    void high_comp_allocation_call(std::shared_ptr<model::WorkItem> item, WorkQueueManager *queueManager) {
        auto processingItem = std::static_pointer_cast<model::HighProcessingItem>(item);
        bool isReallocation = processingItem->isReallocation();

        std::string sourceHost = (*item->getHostList())[0];

        std::map<std::string, std::shared_ptr<model::HighCompResult>> baseResult;

        std::chrono::time_point<std::chrono::system_clock> currentTime =
                std::chrono::system_clock::now() + std::chrono::milliseconds{HIGH_COMP_START_TIME_OFFSET_MS};

        int resourceConfig = 2;
        if ((processingItem->getDeadline() - currentTime) < std::chrono::milliseconds {FTP_LOW_TIME}) {
            resourceConfig = 4;
            if ((processingItem->getDeadline() - currentTime) < std::chrono::milliseconds{FTP_HIGH_TIME}) {
                web::json::value log;
                log["dnn_id"] = web::json::value::string(processingItem->getDnnId());
                queueManager->logManager->add_log((isReallocation) ? enums::LogTypeEnum::HIGH_COMP_REALLOCATION_FAIL
                                                                   : enums::LogTypeEnum::HIGH_COMP_ALLOCATION_FAIL,
                                                  log);
                return;
            }
        }
        std::unique_lock<std::mutex> netlock(queueManager->network_lock, std::defer_lock);
        netlock.lock();

        if (isReallocation) {
            baseResult = processingItem->baseResult;
            for (const auto &[dnn_id, baseRes]: baseResult) {
                baseRes->setVersion(std::chrono::system_clock::now().time_since_epoch().count() * 1000);
            }
        }

        std::vector<int> task_comm_windows = {};
        int task_count = static_cast<int>(processingItem->getDnnIds().size());

        for(int i = 0; i < task_count; i++){
            auto netlink_index = queueManager->network->network_link;
            auto ct_uint = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(currentTime.time_since_epoch()).count());
            auto last_reason_uint = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(queueManager->network->last_time_of_reasoning.time_since_epoch()).count());
            auto bytes_per_millisecond = queueManager->getBytesPerMillisecond();
            auto base_comm_size = static_cast<uint64_t>(TASK_FORWARD_SIZE / bytes_per_millisecond);

            int index = services::obtain_index(ct_uint, last_reason_uint, base_comm_size, BASE_BUCKET_COUNT);
        }




        std::unique_lock<std::mutex> offload_lock(queueManager->offloaded_lock, std::defer_lock);
        offload_lock.lock();

        std::vector<std::chrono::time_point<std::chrono::system_clock>> finish_times;

        for (const auto &[dnn_id, dnn_task]: queueManager->off_total) {
            auto finish = dnn_task->estimated_start_fin->stop;

            auto now = currentTime;
            finish_times.push_back(now);
            std::sort(finish_times.begin(), finish_times.end());

            std::vector<std::string> allocated_tasks;

            for (const auto &finish_time: finish_times) {
                auto allocationMap = utils::generateAllocationMap(queueManager->network->devices);

                std::vector<std::shared_ptr<model::HighCompResult>> results;

                for (const auto &dnn_id: processingItem->getDnnIds()) {
                    if (utils::is_allocated(dnn_id, allocated_tasks))
                        continue;

                    //NEED TO FIND EARLIEST LINK SLOT TO COMMUNICATE DATA TO SOURCE DEVICE
                    auto initial_data_communication_slot = services::findLinkSlot(finish_time, bytes_per_ms,
                                                                                  HIGH_TASK_SIZE,
                                                                                  queueManager->network->network_link);

                    /* We need to create a link activity for the initial upload of data */
                    std::shared_ptr<model::LinkAct> initial_task_allocation = std::make_shared<model::LinkAct>();
                    initial_task_allocation->setIsMeta(false);
                    initial_task_allocation->setDataSize(HIGH_TASK_SIZE);
                    initial_task_allocation->setStartFinTime(
                            std::make_pair(initial_data_communication_slot->first,
                                           initial_data_communication_slot->second));
                    initial_task_allocation->setHostNames(std::make_pair(CONTROLLER_HOSTNAME, sourceHost));

                    //This window is only used if the task is allocated to a device other than its source
                    auto data_transfer_time_base =
                            initial_data_communication_slot->second + std::chrono::milliseconds{1};
                    auto data_transfer_window = services::findLinkSlot(data_transfer_time_base, bytes_per_ms,
                                                                       TASK_FORWARD_SIZE,
                                                                       queueManager->network->network_link);

                    std::shared_ptr<model::LinkAct> data_transfer_link_act = std::make_shared<model::LinkAct>();
                    data_transfer_link_act->setIsMeta(false);
                    data_transfer_link_act->setDataSize(TASK_FORWARD_SIZE);
                    data_transfer_link_act->setStartFinTime(
                            std::make_pair(data_transfer_window->first, data_transfer_window->second));

                    auto src_start_time =
                            initial_task_allocation->getStartFinTime().second + std::chrono::milliseconds{1};
                    auto remote_start_time =
                            data_transfer_link_act->getStartFinTime().second + std::chrono::milliseconds{1};

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

                    auto selected_device = services::findNode(queueManager->network->devices,
                                                              initial_task_allocation->getStartFinTime().second,
                                                              finish_time_on_src,
                                                              data_transfer_link_act->getStartFinTime().second,
                                                              finish_time_on_remote, sourceHost, current_core,
                                                              allocationMap, dnn_id);

                    if (selected_device.first != TASK_NOT_FOUND) {
                        std::shared_ptr<model::HighCompResult> task = std::make_shared<model::HighCompResult>(dnn_id,
                                                                                                              sourceHost,
                                                                                                              processingItem->getDeadline(),
                                                                                                              initial_task_allocation,
                                                                                                              enums::dnn_type::high_comp);
                        if (selected_device.second != sourceHost) {

                            task->setTaskAllocation(data_transfer_link_act);
                            task->setEstimatedStart(remote_start_time);
                            task->setEstimatedFinish(finish_time_on_remote);

                            queueManager->network->addComm(data_transfer_link_act);
                        } else {
                            task->setEstimatedStart(src_start_time);
                            task->setEstimatedFinish(finish_time_on_src);
                        }

                        queueManager->network->addComm(initial_task_allocation);
                        task->setAllocatedHost(selected_device.second);
                        task->setCoreAllocation(current_core);
                        task->setN(CURRENT_N);
                        task->setM(CURRENT_M);

                        queueManager->network->devices[selected_device.second]->DNNS.push_back(task);
                        results.push_back(task);
                        allocationMap[selected_device.second] += 1;
                    }
                }

                for (const auto &task: results) {

                    std::chrono::time_point<std::chrono::system_clock> finish_time_on_device =
                            task->getEstimatedStart() +
                            std::chrono::milliseconds{
                                    FTP_HIGH_TIME};
                    auto core_usage = calculateDeviceCoreUsage(task->getEstimatedStart(), finish_time_on_device,
                                                               queueManager->network->devices[task->getAllocatedHost()],
                                                               task->getDnnId());


                    if (core_usage + FTP_HIGH_CORE <=
                        queueManager->network->devices[task->getAllocatedHost()]->getCores()) {
                        task->setN(FTP_HIGH_N);
                        task->setM(FTP_HIGH_M);
                        task->setCoreAllocation(FTP_HIGH_CORE);
                        task->setEstimatedFinish(finish_time_on_device);
                    }

                    /* If the current allocation does not satisfy deadline,
                     * remove link tasks from link and task from allocated host
                     * so that we can continue to attempt to allocate */
                    if (task->getEstimatedFinish() > processingItem->getDeadline()) {
                        if (task->getAllocatedHost() != task->getSrcHost()) {
                            int comm_id = task->getTaskAllocation()->getLinkActivityId();

                            queueManager->network->network_link.erase(
                                    std::remove_if(queueManager->network->network_link.begin(),
                                                   queueManager->network->network_link.end(),
                                                   [comm_id](std::shared_ptr<model::LinkAct> la) {
                                                       return comm_id == la->getLinkActivityId();
                                                   }), queueManager->network->network_link.end());

                        }

                        int comm_id = task->getUploadData()->getLinkActivityId();
                        queueManager->network->network_link.erase(
                                std::remove_if(queueManager->network->network_link.begin(),
                                               queueManager->network->network_link.end(),
                                               [comm_id](std::shared_ptr<model::LinkAct> la) {
                                                   return comm_id == la->getLinkActivityId();
                                               }), queueManager->network->network_link.end());

                        auto dnn_id = task->getDnnId();
                        auto allocated_device = task->getAllocatedHost();
                        queueManager->network->devices[allocated_device]->DNNS.erase(
                                std::remove_if(queueManager->network->devices[allocated_device]->DNNS.begin(),
                                               queueManager->network->devices[allocated_device]->DNNS.end(),
                                               [dnn_id](std::shared_ptr<model::BaseCompResult> br) {
                                                   return dnn_id == br->getDnnId();
                                               }), queueManager->network->devices[allocated_device]->DNNS.end());

                    } else {
                        auto state_update_base = task->getEstimatedFinish() + std::chrono::milliseconds{1};
                        auto state_update_window = services::findLinkSlot(state_update_base, float(bytes_per_ms),
                                                                          STATE_UPDATE_SIZE,
                                                                          queueManager->network->network_link);

                        std::shared_ptr<model::LinkAct> state_update_link_act = std::make_shared<model::LinkAct>();
                        state_update_link_act->setIsMeta(false);
                        state_update_link_act->setDataSize(STATE_UPDATE_SIZE);
                        state_update_link_act->setStartFinTime(
                                std::make_pair(state_update_window->first, state_update_window->second));

                        task->setStateUpdate(state_update_link_act);
                        queueManager->network->addComm(state_update_link_act);

                        queueManager->off_high[task->getDnnId()] = task;
                        queueManager->off_total[task->getDnnId()] = std::static_pointer_cast<model::BaseCompResult>(
                                task);

                        std::shared_ptr<model::HighComplexityAllocationComms> high_comp_comms = std::make_shared<model::HighComplexityAllocationComms>(
                                enums::network_comms_types::high_complexity_task_mapping,
                                task->getUploadData()->getStartFinTime().first,
                                task,
                                task->getSrcHost());

                        std::shared_ptr<model::BaseNetworkCommsModel> comm_task = std::static_pointer_cast<model::BaseNetworkCommsModel>(
                                high_comp_comms);

                        web::json::value log;
                        log["dnn"] =
                                web::json::value(task->convertToJson());
                        queueManager->logManager->
                                add_log((isReallocation)
                                        ? enums::LogTypeEnum::HIGH_COMP_REALLOCATION_SUCCESS
                                        : enums::LogTypeEnum::HIGH_COMP_ALLOCATION_SUCCESS, log);

                        queueManager->networkQueueManager->
                                addTask(comm_task);

                        allocated_tasks.push_back(task->getDnnId());
                    }
                }

                if (allocated_tasks.size() == processingItem->getDnnIds().size())
                    break;
            }

            for (const auto &dnn_id: processingItem->getDnnIds()) {
                bool allocated = false;
                for (const auto &dnn_task: allocated_tasks) {
                    if (dnn_id == dnn_task) {
                        allocated = true;
                        break;
                    }
                }
                if (!allocated) {
                    web::json::value log;
                    log["dnn_id"] = web::json::value::string(dnn_id);
                    queueManager->logManager->add_log((isReallocation) ? enums::LogTypeEnum::HIGH_COMP_REALLOCATION_FAIL
                                                                       : enums::LogTypeEnum::HIGH_COMP_ALLOCATION_FAIL,
                                                      log);
                }
            }
            netlock.unlock();
            offload_lock.unlock();
            queueManager->decrementThreadCounter();
        }

        void halt_call(std::shared_ptr<model::WorkItem> workItem, WorkQueueManager *queueManager) {
            auto haltWorkItem = std::static_pointer_cast<model::HaltWorkItem>(workItem);
            std::vector<std::string> hostList;
            for (auto [host_id, host]: queueManager->network->getDevices()) {
                hostList.push_back(host_id);
            }

            auto sourceDeviceId = haltWorkItem->getHostToExamine();
            auto startTime = haltWorkItem->getStartTime();
            auto finTime = haltWorkItem->getFinTime();

            std::unique_lock<std::mutex> offload_lock(queueManager->offloaded_lock, std::defer_lock);
            std::unique_lock<std::mutex> network_lock(queueManager->network_lock, std::defer_lock);
            offload_lock.lock();
            network_lock.lock();

            auto sourceDevice = queueManager->network->devices[sourceDeviceId];
            auto tasks = sourceDevice->DNNS;

            std::vector<std::pair<int, std::shared_ptr<model::HighCompResult>>> haltCandidates;

            for (int i = 0; i < tasks.size(); i++) {
                if ((max(startTime, tasks[i]->getEstimatedStart()) -
                     min(finTime, tasks[i]->getEstimatedFinish())).count() <= 0)
                    haltCandidates.emplace_back(i, std::static_pointer_cast<model::HighCompResult>(tasks[i]));
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

            auto dnn_id = dnnToPrune->getDnnId();
            auto allocated_device = dnnToPrune->getAllocatedHost();
            queueManager->network->devices[allocated_device]->DNNS.erase(
                    std::remove_if(queueManager->network->devices[allocated_device]->DNNS.begin(),
                                   queueManager->network->devices[allocated_device]->DNNS.end(),
                                   [dnn_id](std::shared_ptr<model::BaseCompResult> br) {
                                       return dnn_id == br->getDnnId();
                                   }), queueManager->network->devices[allocated_device]->DNNS.end());

            auto versionToPrune = dnnToPrune->getVersion();

            dnnToPrune->setCoreAllocation(0);
            dnnToPrune->setEstimatedFinish(
                    std::chrono::time_point<std::chrono::system_clock>(std::chrono::milliseconds{0}));
            dnnToPrune->setEstimatedStart(
                    std::chrono::time_point<std::chrono::system_clock>(std::chrono::milliseconds{0}));
            dnnToPrune->setVersion(std::chrono::system_clock::now().time_since_epoch().count() * 1000);
            dnnToPrune->setN(0);
            dnnToPrune->setM(0);

            std::vector<std::shared_ptr<model::LinkAct>> links_to_prune;
            links_to_prune.push_back(dnnToPrune->getUploadData());
            links_to_prune.push_back(dnnToPrune->getStateUpdate());
            if (dnnToPrune->getSrcHost() == dnnToPrune->getAllocatedHost())
                links_to_prune.push_back(dnnToPrune->getTaskAllocation());

            for (const auto &prune_link: links_to_prune) {
                auto link_id = prune_link->getLinkActivityId();
                queueManager->network->network_link.erase(std::remove_if(queueManager->network->network_link.begin(),
                                                                         queueManager->network->network_link.end(),
                                                                         [link_id](
                                                                                 std::shared_ptr<model::LinkAct> linkAct) {
                                                                             return linkAct->getLinkActivityId() ==
                                                                                    link_id;
                                                                         }), queueManager->network->network_link.end());
            }

            dnnToPrune->setAllocatedHost("");

            queueManager->off_total.erase(dnnToPrune->getDnnId());
            queueManager->off_high.erase(dnnToPrune->getDnnId());

            std::shared_ptr<model::HaltNetworkCommsModel> baseNetworkCommsModel = std::make_shared<model::HaltNetworkCommsModel>(
                    enums::network_comms_types::halt_req,
                    std::chrono::system_clock::now(), sourceDevice->getHostName(), dnnToPrune->getDnnId(),
                    versionToPrune);

            queueManager->networkQueueManager->addTask(baseNetworkCommsModel);

            network_lock.unlock();
            offload_lock.unlock();

            /* We attempt to reallocate the DNN if possible */
            auto host_list = std::make_shared<std::vector<std::string>>(
                    std::initializer_list<std::string>{dnnToPrune->getSrcHost()});

            std::shared_ptr<model::HighProcessingItem> highProcessingItem = std::make_shared<model::HighProcessingItem>(
                    host_list,
                    enums::request_type::high_complexity,
                    dnnToPrune->getDeadline(),
                    std::string(dnnToPrune->getDnnId()), std::initializer_list<std::string>{dnnToPrune->getDnnId()});

            highProcessingItem->setReallocation(true);
            queueManager->add_task(highProcessingItem);


            queueManager->decrementThreadCounter();
        }

        void WorkQueueManager::add_task(std::shared_ptr<model::WorkItem> item) {
            web::json::value log;
            std::unique_lock<std::mutex> lk(WorkQueueManager::work_queue_lock, std::defer_lock);
            lk.lock();
            WorkQueueManager::logManager->add_log(enums::LogTypeEnum::ADD_WORK_TASK, log);
            WorkQueueManager::work_queue.push_back(item);

            std::sort(WorkQueueManager::work_queue.begin(), WorkQueueManager::work_queue.end(),
                      [](std::shared_ptr<model::WorkItem> a, std::shared_ptr<model::WorkItem> b) {

                          if (a->getRequestType() == b->getRequestType()) {
                              if (a->getRequestType() == enums::request_type::low_complexity) {
                                  auto a_cast = std::static_pointer_cast<model::LowProcessingItem>(a);
                                  auto b_cast = std::static_pointer_cast<model::LowProcessingItem>(b);

                                  return a_cast->getDeadline() < b_cast->getDeadline();
                              } else if (a->getRequestType() == enums::request_type::high_complexity) {
                                  auto a_cast = std::static_pointer_cast<model::HighProcessingItem>(a);
                                  auto b_cast = std::static_pointer_cast<model::HighProcessingItem>(b);

                                  return a_cast->getDeadline() < b_cast->getDeadline();
                              }
                              return true;
                          } else
                              return a->getRequestType() < b->getRequestType();
                      });
            lk.unlock();
        }

        [[noreturn]] void WorkQueueManager::main_loop(WorkQueueManager *queueManager) {
            std::chrono::time_point<std::chrono::system_clock> next_discretisation_task;
            std::random_device rd;
            std::mt19937 gen(rd());
            try {
                while (queueManager->networkQueueManager->hosts.size() < CLIENT_COUNT)
                    continue;
                next_discretisation_task =
                        std::chrono::system_clock::now() + std::chrono::milliseconds{NETWORK_PING_FREQ_MS};

                std::unique_lock<std::mutex> lk(queueManager->work_queue_lock, std::defer_lock);
                while (true) {

                    queueManager->current_task.clear();
                    queueManager->thread_counter = 0;

                    if (!queueManager->work_queue.empty()) {
                        lk.lock();

                        queueManager->thread_counter++;
                        queueManager->current_task.push_back(queueManager->work_queue.front());

                        queueManager->work_queue.erase(queueManager->work_queue.begin());

                        lk.unlock();

                        std::vector<std::thread> thread_pool;
                        switch (queueManager->current_task.front()->getRequestType()) {
                            case enums::request_type::low_complexity:
                                thread_pool.emplace_back(low_comp_allocation_call, queueManager->current_task.front(),
                                                         queueManager);
                                break;
                            case enums::request_type::high_complexity:
                                thread_pool.emplace_back(high_comp_allocation_call, queueManager->current_task.front(),
                                                         queueManager);
                                break;
                            case enums::request_type::halt_req:
                                thread_pool.emplace_back(halt_call, queueManager->current_task.front(), queueManager);
                                break;
                            case enums::request_type::state_update:
                                thread_pool.emplace_back(state_update_call, queueManager->current_task.front(),
                                                         queueManager);
                                break;
                            case enums::request_type::bandwidth_update:
                                thread_pool.emplace_back(update_bw_vals, queueManager->current_task.front(),
                                                         queueManager);
                                break;
                            case enums::request_type::network_disc:
                                thread_pool.emplace_back(update_network_disc, queueManager->current_task.front(),
                                                         queueManager);
                                break;
                        }
                        thread_pool.back().detach();
                        /* We wait for the current work items to terminate */
                        while (queueManager->thread_counter > 0) {
                            lk.lock();
                            if (!queueManager->work_queue.empty() &&
                                queueManager->current_task.front()->getRequestType() ==
                                enums::request_type::low_complexity &&
                                queueManager->work_queue.front()->getRequestType() ==
                                enums::request_type::low_complexity) {
                                /* If more low complexity tasks are added to the queue then we append them to the current
                                 * work list */
                                while (!queueManager->work_queue.empty() &&
                                       queueManager->work_queue.front()->getRequestType() ==
                                       enums::request_type::low_complexity) {
                                    queueManager->thread_counter++;
                                    thread_pool.emplace_back(low_comp_allocation_call, queueManager->work_queue.front(),
                                                             queueManager);
                                    thread_pool.back().detach();
                                    queueManager->current_task.push_back(queueManager->work_queue.front());
                                    queueManager->work_queue.erase(queueManager->work_queue.begin());
                                }
                            }
                            lk.unlock();
                        }
                        thread_pool.clear();

                    }

                    if (std::chrono::system_clock::now() > next_discretisation_task) {
                        lk.lock();

                        auto network_device_list = queueManager->networkQueueManager->hosts;

                        std::uniform_int_distribution<> dis(0, static_cast<int>(network_device_list.size()) -
                                                               1); // Range from 0 to vec.size()-1

                        auto chosen_host = network_device_list[dis(gen)];

                        std::shared_ptr<model::BandwidthTestCommsModel> bandwidth_test = std::make_shared<model::BandwidthTestCommsModel>(
                                enums::network_comms_types::bandwidth_update, std::chrono::system_clock::now(),
                                chosen_host);

                        queueManager->networkQueueManager->addTask(
                                std::static_pointer_cast<model::BaseNetworkCommsModel>(bandwidth_test));
                        next_discretisation_task =
                                std::chrono::system_clock::now() + std::chrono::milliseconds{NETWORK_PING_FREQ_MS};
                        lk.unlock();
                    }
                }

            }
            catch (std::exception &e) {
                std::cerr << "WORK_QUEUE_MANAGER: something wrong has happened! ;)" << '\n';
                std::cerr << e.what() << "\n";
            }
        }

        void WorkQueueManager::decrementThreadCounter() {
            WorkQueueManager::thread_counter--;
        }

        double WorkQueueManager::getAverageBitsPerSecond() const {
            return average_bits_per_second;
        }

        void WorkQueueManager::setAverageBitsPerSecond(double averageBitsPerSecond) {
            average_bits_per_second = averageBitsPerSecond;
        }

        double WorkQueueManager::getJitter() const {
            return jitter;
        }

        void WorkQueueManager::setJitter(double jitter) {
            WorkQueueManager::jitter = jitter;
        }

        WorkQueueManager::WorkQueueManager(std::shared_ptr<LogManager>
        ptr, std::shared_ptr<NetworkQueueManager>
        sharedPtr)
        : logManager(std::move(ptr))
        {
            WorkQueueManager::network = std::make_shared<model::Network>();
            WorkQueueManager::networkQueueManager = sharedPtr;

        }

        double WorkQueueManager::getBytesPerMillisecond() {
            /* Fetching the bandwidth and latency */
            double bw_bytes_per_second = (WorkQueueManager::getAverageBitsPerSecond() / 8);
            double latency_bytes = WorkQueueManager::getJitter() / 8;
            bw_bytes_per_second -= latency_bytes;
            double bw_bytes_per_ms = bw_bytes_per_second / 1000;
            return bw_bytes_per_ms;
        }
    } // services