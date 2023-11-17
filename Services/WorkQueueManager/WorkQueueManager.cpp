//
// Created by jamiec on 9/23/22.
//

#include <thread>
#include <utility>
#include "WorkQueueManager.h"
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
#include "../../model/data_models/WorkItems/WorkRequest/WorkRequest.h"
#include "../../model/data_models/NetworkCommsModels/WorkRequestResponse/WorkRequestResponse.h"

namespace services {
    std::atomic<int> WorkQueueManager::thread_counter = 0;

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


        /* Begin iterating through the DNN to gather tasks and links to remove */
        std::vector<std::shared_ptr<model::LinkAct>> links_to_prune;

        if(dnn->getDnnType() == enums::dnn_type::low_comp) {
            links_to_prune.push_back(dnn->getUploadData());
            links_to_prune.push_back(dnn->getStateUpdate());
        }

        for (const auto &prune_link: links_to_prune) {
            auto link_id = prune_link->getLinkActivityId();
            queueManager->network->network_link.erase(std::remove_if(queueManager->network->network_link.begin(),
                                                                     queueManager->network->network_link.end(),
                                                                     [link_id](
                                                                             std::shared_ptr<model::LinkAct> linkAct) {
                                                                         return linkAct->getLinkActivityId() == link_id;
                                                                     }), queueManager->network->network_link.end());
        }

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

        /*Need to create a copy of the network list so that we can keep track of incomplete net allocations */
        std::vector<std::shared_ptr<model::LinkAct>> copyList;
        for (const auto &i: queueManager->network->network_link) {
            copyList.push_back(i);
        }

        net_lock.unlock();

        /* Fetching bytes and latency from the stored net parameters from iperf test */
        double bw_bytes = (queueManager->getAverageBitsPerSecond() / 8);
        double latency_bytes = queueManager->getJitter() / 8;
        bw_bytes += latency_bytes;

        uint64_t data_size_bytes = queueManager->state_size;
        /* -------------------------------------------------------------------------- */

        /* Need to find the earliest window for transferring data to host */
        auto [dnn_id, host] = proc_item->getDnnIdAndDevice();

        std::chrono::time_point<std::chrono::system_clock> currentTime = std::chrono::system_clock::now();

        net_lock.lock();

        auto times = services::findLinkSlot(
                currentTime, float(bw_bytes), LOW_TASK_SIZE, copyList);

        net_lock.unlock();
        std::shared_ptr<model::LinkAct> state_transfer = std::make_shared<model::LinkAct>(
                std::make_pair(times->first, times->second));
        state_transfer->setHostNames(std::make_pair(CONTROLLER_HOSTNAME, host));
        state_transfer->setDataSize(data_size_bytes);
        state_transfer->setIsMeta(false);
        copyList.push_back(state_transfer);

        std::sort(copyList.begin(), copyList.end(),
                  [](const std::shared_ptr<model::LinkAct> &a, const std::shared_ptr<model::LinkAct> &b) {
                      return a->getStartFinTime().second < b->getStartFinTime().second;
                  });

        net_lock.lock();

        /* Using the input upload window for each task generated we now allocate a time slot on their respective hosts */
        auto result = services::allocate_task(item, devices, times);
        auto time_window = result.second;
        net_lock.unlock();
        /* If we cannot allocate a device for even one task we instead create a new allocation request and a halt request */
        if (!result.first) {
#if DEADLINE_PREMPT
            std::shared_ptr<model::WorkItem> workItem = std::make_shared<model::HaltWorkItem>(
                    enums::request_type::halt_req, host, time_window->first,
                    time_window->second);

            queueManager->add_task(workItem);
            auto newTask = std::make_shared<model::LowProcessingItem>(item->getHostList(),
                                                                      enums::request_type::low_complexity,
                                                                      proc_item->getDeadline(),
                                                                      proc_item->getDnnIdAndDevice());
            newTask->setReallocation(true);
            queueManager->add_task(std::static_pointer_cast<model::WorkItem>(newTask));

            web::json::value halt_log;
            halt_log["dnn_id"] = web::json::value::string(newTask->getDnnIdAndDevice().first);
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
            auto bR = std::make_shared<model::LowCompResult>(dnn_id, host, 1, deadline, time_window->first,
                                                             time_window->second, state_transfer,
                                                             enums::dnn_type::low_comp
            );

            auto state_times = services::findLinkSlot(
                    time_window->second + std::chrono::milliseconds{1}, float(bw_bytes), STATE_UPDATE_SIZE, copyList);

            std::shared_ptr<model::LinkAct> state_update = std::make_shared<model::LinkAct>(
                    std::make_pair(state_times->first, state_times->second));
            state_transfer->setHostNames(std::make_pair(host, CONTROLLER_HOSTNAME));
            state_transfer->setDataSize(STATE_UPDATE_SIZE);
            state_transfer->setIsMeta(true);

            bR->setStateUpdate(state_update);

            std::unique_lock<std::mutex> offload_lock(queueManager->offloaded_lock, std::defer_lock);
            offload_lock.lock();

            queueManager->off_low[bR->getDnnId()] = bR;
            queueManager->off_total[bR->getDnnId()] = bR;
            offload_lock.unlock();

            /* Add communication times to the network link */
            net_lock.lock();
            queueManager->network->addComm(state_transfer);
            queueManager->network->addComm(state_update);

            bR->setAllocatedHost(host);

            std::shared_ptr<model::LowComplexityAllocationComms> baseNetworkCommsModel =
                    std::make_shared<model::LowComplexityAllocationComms>(
                            enums::network_comms_types::low_complexity_allocation,
                            bR->getUploadData()->getStartFinTime().first,
                            bR,
                            bR->getSrcHost());

            queueManager->network->devices[bR->getSrcHost()]->DNNS.push_back(bR);
            web::json::value log;
            log["dnn_details"] = bR->convertToJson();
            queueManager->logManager->add_log(
                    proc_item->isReallocation() ? enums::LogTypeEnum::LOW_COMP_PREMPT_ALLOCATION_SUCCESS
                                                : enums::LogTypeEnum::LOW_COMP_ALLOCATION_SUCCESS, log);
            queueManager->networkQueueManager->addTask(baseNetworkCommsModel);
            net_lock.unlock();
        }
        queueManager->decrementThreadCounter();
    }

    void add_high_comp_work_item(std::shared_ptr<model::WorkItem> item, WorkQueueManager *queueManager) {
        std::unique_lock<std::mutex> offload_lock(queueManager->offloaded_lock, std::defer_lock);
        offload_lock.lock();

        auto currentTime = std::chrono::system_clock::now();
        auto highProcessingItem = std::static_pointer_cast<model::HighProcessingItem>(item);
        auto work_stealing_queue = queueManager->getWorkStealingQueue();
        work_stealing_queue.push_back(highProcessingItem);
        std::sort(work_stealing_queue.begin(), work_stealing_queue.end(),
                  [](std::shared_ptr<model::HighProcessingItem> a, std::shared_ptr<model::HighProcessingItem> b) {
                      return a->getDeadline() < b->getDeadline();
                  });

        uint64_t comm_bytes = HIGH_TASK_SIZE;
        auto bytes_per_ms = queueManager->getBytesPerMillisecond();
        auto comm_time = static_cast<uint64_t>(comm_bytes / bytes_per_ms);
        auto ftp_high_processing_window = currentTime + std::chrono::milliseconds(comm_time + FTP_HIGH_TIME);

        std::vector<std::shared_ptr<model::HighProcessingItem>> newList;

        for(const auto& task: work_stealing_queue){
            if (ftp_high_processing_window < task->getDeadline())
                newList.push_back(task);
        }

        queueManager->setWorkStealingQueue(newList);
        offload_lock.unlock();

        queueManager->decrementThreadCounter();
    }

    void high_comp_allocation_call(std::shared_ptr<model::WorkItem> item, WorkQueueManager *queueManager) {
        auto workRequest = std::static_pointer_cast<model::WorkRequest>(item);

        int device_capacity = MAX_CORE_ALLOWANCE - workRequest->getCapacity();

        auto currentTime = std::chrono::system_clock::now();
        std::string sourceHost = (*item->getHostList())[0];

        if (device_capacity < FTP_LOW_CORE) {
            std::shared_ptr<model::HighComplexityAllocationComms> high_comp_comms = std::make_shared<model::HighComplexityAllocationComms>(
                    enums::network_comms_types::high_complexity_task_mapping,
                    currentTime,
                    false, sourceHost);

            std::shared_ptr<model::BaseNetworkCommsModel> work_request_response = std::static_pointer_cast<model::BaseNetworkCommsModel>(
                    high_comp_comms);

            queueManager->networkQueueManager->addTask(work_request_response);
            web::json::value log;
            log["host_request"] = web::json::value(sourceHost);
            log["host_capacity"] = web::json::value::number(device_capacity);
            queueManager->logManager->add_log(enums::LogTypeEnum::HIGH_COMP_ALLOCATION_FAIL, log);

            queueManager->decrementThreadCounter();
            return;
        }

        auto bytes_per_ms = queueManager->getBytesPerMillisecond();

        std::unique_lock<std::mutex> netlock(queueManager->network_lock, std::defer_lock);
        std::unique_lock<std::mutex> offload_lock(queueManager->offloaded_lock, std::defer_lock);
        offload_lock.lock();
        netlock.lock();

        std::shared_ptr<model::HighCompResult> result;

        auto work_queue = queueManager->getWorkStealingQueue();

        int index = 0;

        int CURRENT_N = -1;
        int CURRENT_M = -1;


        std::shared_ptr<model::HighProcessingItem> selectedTask;
        std::chrono::time_point<std::chrono::system_clock> finish_time;
        std::chrono::time_point<std::chrono::system_clock> start_time;
        for (index = 0; index < work_queue.size(); index++) {

            uint64_t comm_bytes = HIGH_TASK_SIZE;
            if ((*work_queue[index]->getHostList())[0] != sourceHost)
                comm_bytes += TASK_FORWARD_SIZE;

            auto comm_time = static_cast<uint64_t>(comm_bytes / bytes_per_ms);
            auto ftp_low_processing_window = currentTime + std::chrono::milliseconds(comm_time + FTP_LOW_TIME);
            auto ftp_high_processing_window = currentTime + std::chrono::milliseconds(comm_time + FTP_HIGH_TIME);

            if (ftp_low_processing_window < work_queue[index]->getDeadline()) {
                CURRENT_N = FTP_LOW_N;
                CURRENT_M = FTP_LOW_M;
                selectedTask = work_queue[index];
                start_time = currentTime + std::chrono::milliseconds{comm_time};
                finish_time = ftp_low_processing_window;
                break;
            } else if (device_capacity == FTP_HIGH_CORE &&
                       ftp_high_processing_window < work_queue[index]->getDeadline()) {
                CURRENT_N = FTP_HIGH_N;
                CURRENT_M = FTP_HIGH_M;
                selectedTask = work_queue[index];
                start_time = currentTime + std::chrono::milliseconds{comm_time};
                finish_time = ftp_high_processing_window;
                break;
            }
        }

        if (CURRENT_M != TASK_NOT_FOUND) {
            work_queue.erase(work_queue.begin() + index);
            std::shared_ptr<model::HighCompResult> task = std::make_shared<model::HighCompResult>(
                    selectedTask->getDnnId(),
                    (*selectedTask->getHostList())[0],
                    selectedTask->getDeadline(),
                    enums::dnn_type::high_comp, CURRENT_N, CURRENT_M);

            task->setAllocatedHost(sourceHost);
            task->setEstimatedStart(start_time);
            task->setEstimatedFinish(finish_time);

            queueManager->off_high[task->getDnnId()] = task;
            queueManager->off_total[task->getDnnId()] = std::static_pointer_cast<model::BaseCompResult>(task);
            std::shared_ptr<model::WorkRequestResponse> workRequestResponse = std::make_shared<model::WorkRequestResponse>(
                    enums::network_comms_types::work_request_response,
                    currentTime,
                    true,
                    task->getN() * task->getM(),
                    sourceHost);

            std::shared_ptr<model::HighComplexityAllocationComms> high_comp_comms = std::make_shared<model::HighComplexityAllocationComms>(
                    enums::network_comms_types::high_complexity_task_mapping,
                    currentTime,
                    task,
                    task->getAllocatedHost(), true);


            queueManager->network->devices[task->getAllocatedHost()]->DNNS.push_back(task);

            std::shared_ptr<model::BaseNetworkCommsModel> comm_task = std::static_pointer_cast<model::BaseNetworkCommsModel>(
                    high_comp_comms);

            queueManager->networkQueueManager->addTask(comm_task);

            web::json::value log;
            log["dnn"] = web::json::value(task->convertToJson());
            log["host_request"] = web::json::value(sourceHost);
            log["host_capacity"] = web::json::value::number(device_capacity);
            queueManager->logManager->add_log(enums::LogTypeEnum::HIGH_COMP_ALLOCATION_SUCCESS, log);

        }
        else{
            std::shared_ptr<model::HighComplexityAllocationComms> high_comp_comms = std::make_shared<model::HighComplexityAllocationComms>(
                    enums::network_comms_types::high_complexity_task_mapping,
                    currentTime,
                    false, sourceHost);

            std::shared_ptr<model::BaseNetworkCommsModel> work_request_response = std::static_pointer_cast<model::BaseNetworkCommsModel>(
                    high_comp_comms);

            web::json::value log;
            log["host_request"] = web::json::value(sourceHost);
            log["host_capacity"] = web::json::value::number(device_capacity);
            queueManager->logManager->add_log(enums::LogTypeEnum::HIGH_COMP_ALLOCATION_FAIL, log);

            queueManager->networkQueueManager->addTask(work_request_response);
        }

        queueManager->setWorkStealingQueue(work_queue);

        netlock.unlock();
        offload_lock.unlock();
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
                dnnToPrune->getDnnId());

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
        try {
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
                            thread_pool.emplace_back(add_high_comp_work_item, queueManager->current_task.front(),
                                                     queueManager);
                            break;
                        case enums::request_type::halt_req:
                            thread_pool.emplace_back(halt_call, queueManager->current_task.front(), queueManager);
                            break;
                        case enums::request_type::state_update:
                            thread_pool.emplace_back(state_update_call, queueManager->current_task.front(),
                                                     queueManager);
                            break;
                    }
                    thread_pool.back().detach();
                    /* We wait for the current work items to terminate */
                    while (queueManager->thread_counter > 0) {
                        lk.lock();
                        if (!queueManager->work_queue.empty() && queueManager->current_task.front()->getRequestType() ==
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
            : logManager(std::move(ptr)) {
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

    const std::vector<std::shared_ptr<model::HighProcessingItem>> &WorkQueueManager::getWorkStealingQueue() const {
        return work_stealing_queue;
    }

    void WorkQueueManager::setWorkStealingQueue(
            const std::vector<std::shared_ptr<model::HighProcessingItem>> &workStealingQueue) {
        work_stealing_queue = workStealingQueue;
    }

} // services