//
// Created by jamiec on 9/23/22.
//

#include <thread>
#include <utility>
#include "WorkQueueManager.h"
#include "../LowCompServices/LowCompServices.h"
#include "../../model/data_models/WorkItems/StateUpdate/StateUpdate.h"
#include "../NetworkServices/NetworkServices.h"
#include "../../Constants/ModeMacros.h"
#include "../../utils/UtilFunctions/UtilFunctions.h"
#include "../../Constants/CLIENT_DETAILS.h"
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

using namespace web;

namespace services {

    void WorkQueueManager::state_update_call(std::shared_ptr<model::WorkItem> item) {
        try {
            std::shared_ptr<model::StateUpdate> stateUpdate = std::static_pointer_cast<model::StateUpdate>(item);

            auto work_type = stateUpdate->getRequestType();

            auto dnn_id = stateUpdate->getDnnId();
            if (!WorkQueueManager::off_total.count(dnn_id)) {
                return;
            }

            auto dnn = WorkQueueManager::off_total[dnn_id];

            std::string allocated_device = dnn->getAllocatedHost();

            WorkQueueManager::network->devices[allocated_device]->DNNS.erase(
                    std::remove_if(WorkQueueManager::network->devices[allocated_device]->DNNS.begin(),
                                   WorkQueueManager::network->devices[allocated_device]->DNNS.end(),
                                   [dnn_id](std::shared_ptr<model::BaseCompResult> br) {
                                       return dnn_id == br->getDnnId();
                                   }), WorkQueueManager::network->devices[allocated_device]->DNNS.end());


            /* Begin iterating through the DNN to gather tasks and links to remove */
            std::vector<std::shared_ptr<model::LinkAct>> links_to_prune;

            if (dnn->getDnnType() == enums::dnn_type::low_comp) {
                links_to_prune.push_back(dnn->getUploadData());
                links_to_prune.push_back(dnn->getStateUpdate());
            }

            for (const auto &prune_link: links_to_prune) {
                auto link_id = prune_link->getLinkActivityId();

                auto new_network_link = std::vector<std::shared_ptr<model::LinkAct>>();

                std::copy_if(WorkQueueManager::network->network_link.begin(),
                             WorkQueueManager::network->network_link.end(), std::back_inserter(new_network_link),
                             [link_id](std::shared_ptr<model::LinkAct> res) {
                                 return res->getLinkActivityId() != link_id;
                             });

                WorkQueueManager::network->network_link = new_network_link;
            }

            auto new_off_total = std::map<std::string, std::shared_ptr<model::BaseCompResult>>();

            std::copy_if(WorkQueueManager::off_total.begin(), WorkQueueManager::off_total.end(),
                         std::inserter(new_off_total, new_off_total.end()),
                         [dnn_id](const std::pair<std::string, std::shared_ptr<model::BaseCompResult>> &res) {
                             return res.first != dnn_id;
                         });

            WorkQueueManager::off_total = new_off_total;


            if (work_type != enums::request_type::return_task && stateUpdate->isSuccess())
                dnn->setActualFinish(stateUpdate->getFinishTime());
            web::json::value log;

            if (work_type != enums::request_type::return_task) {
                if (dnn->getDnnType() == enums::dnn_type::low_comp) {
                    log["dnn"] = std::static_pointer_cast<model::LowCompResult>(dnn)->convertToJson();
                    WorkQueueManager::logManager->add_log(enums::LogTypeEnum::LOW_COMP_FINISH, log);
                } else {
                    log["dnn"] = std::static_pointer_cast<model::HighCompResult>(dnn)->convertToJson();
                    WorkQueueManager::logManager->add_log(
                            stateUpdate->isSuccess() ? enums::LogTypeEnum::HIGH_COMP_FINISH
                                                     : enums::LogTypeEnum::VIOLATED_DEADLINE,
                            log);
                }
            }


        }
        catch (std::exception &e) {
            std::cout << "WorkQueueManager, State Update: Crash" << e.what() << std::endl;
        }
    }

    /*Function receives alow complexity DNN allocation request*/
    void WorkQueueManager::low_comp_allocation_call(std::shared_ptr<model::WorkItem> item) {
        try {
            auto proc_item = std::static_pointer_cast<model::LowProcessingItem>(item);

            std::map<std::string, std::shared_ptr<model::ComputationDevice>> &devices = WorkQueueManager::network->devices;

            /*Need to create a copy of the network list so that we can keep track of incomplete net allocations */
            std::vector<std::shared_ptr<model::LinkAct>> copyList;
            for (const auto &i: WorkQueueManager::network->network_link) {
                copyList.push_back(i);
            }

            /* Fetching bytes and latency from the stored net parameters from iperf test */
            double bw_bytes = (WorkQueueManager::getAverageBitsPerSecond() / 8);
            double latency_bytes = WorkQueueManager::getJitter() / 8;
            bw_bytes += latency_bytes;

            uint64_t data_size_bytes = WorkQueueManager::state_size;
            /* -------------------------------------------------------------------------- */

            /* Need to find the earliest window for transferring data to host */
            auto [dnn_id, host] = proc_item->getDnnIdAndDevice();

            std::chrono::time_point<std::chrono::system_clock> currentTime = std::chrono::system_clock::now();

            auto times = services::findLinkSlot(
                    currentTime, float(bw_bytes), LOW_TASK_SIZE, copyList);

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


            /* Using the input upload window for each task generated we now allocate a time slot on their respective hosts */
            auto result = services::allocate_task(item, devices, times);
            auto time_window = result.second;

            /* If we cannot allocate a device for even one task we instead create a new allocation request and a halt request */
            if (!result.first) {
#if DEADLINE_PREMPT
                std::shared_ptr<model::WorkItem> workItem = std::make_shared<model::HaltWorkItem>(
                        enums::request_type::halt_req, host);

                WorkQueueManager::add_task(workItem);
                auto newTask = std::make_shared<model::LowProcessingItem>(item->getHostList(),
                                                                          enums::request_type::low_complexity,
                                                                          proc_item->getDeadline(),
                                                                          proc_item->getDnnIdAndDevice());
                newTask->setReallocation(true);
                WorkQueueManager::add_task(std::static_pointer_cast<model::WorkItem>(newTask));

                web::json::value halt_log;
                halt_log["dnn_id"] = web::json::value::string(newTask->getDnnIdAndDevice().first);
                WorkQueueManager::logManager->add_log(enums::LogTypeEnum::HALT_REQUEST, halt_log);
#endif
                web::json::value log;
                log["dnn_id"] = web::json::value::string(proc_item->getDnnIdAndDevice().first);
                log["current_time"] = web::json::value::number(
                        std::chrono::system_clock::now().time_since_epoch().count() * 1000);
                log["source_device"] = web::json::value::string(host);
                log["deadline"] = web::json::value::number(proc_item->getDeadline().time_since_epoch().count() * 1000);
                log["network"] = WorkQueueManager::network->convertToJson();
                WorkQueueManager::logManager->add_log(enums::LogTypeEnum::LOW_COMP_ALLOCATION_FAIL, log);
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
                        time_window->second + std::chrono::milliseconds{1}, float(bw_bytes), STATE_UPDATE_SIZE,
                        copyList);

                std::shared_ptr<model::LinkAct> state_update = std::make_shared<model::LinkAct>(
                        std::make_pair(state_times->first, state_times->second));
                state_transfer->setHostNames(std::make_pair(host, CONTROLLER_HOSTNAME));
                state_transfer->setDataSize(STATE_UPDATE_SIZE);
                state_transfer->setIsMeta(true);

                bR->setStateUpdate(state_update);

                WorkQueueManager::off_total[bR->getDnnId()] = bR;


                /* Add communication times to the network link */
                WorkQueueManager::network->addComm(state_transfer);
                WorkQueueManager::network->addComm(state_update);

                bR->setAllocatedHost(host);

                std::shared_ptr<model::LowComplexityAllocationComms> baseNetworkCommsModel =
                        std::make_shared<model::LowComplexityAllocationComms>(
                                enums::network_comms_types::low_complexity_allocation,
                                bR->getUploadData()->getStartFinTime().first,
                                bR,
                                bR->getSrcHost());

                WorkQueueManager::network->devices[bR->getSrcHost()]->DNNS.push_back(bR);
                WorkQueueManager::network->devices[bR->getSrcHost()]->setLastLowCompId(bR->getDnnId());

                web::json::value log;
                log["dnn"] = bR->convertToJson();
                WorkQueueManager::logManager->add_log(
                        proc_item->isReallocation() ? enums::LogTypeEnum::LOW_COMP_PREMPT_ALLOCATION_SUCCESS
                                                    : enums::LogTypeEnum::LOW_COMP_ALLOCATION_SUCCESS, log);
                services::lowTaskAllocation(baseNetworkCommsModel, WorkQueueManager::logManager);
            }
        }
        catch (std::exception &e) {
            std::cout << "WorkQueueManager, Low Comp Allocation: Crash" << e.what() << std::endl;
        }
    }

    void WorkQueueManager::add_high_comp_work_item(std::shared_ptr<model::WorkItem> item) {
        try {
            auto currentTime = std::chrono::system_clock::now();
            auto highProcessingItem = std::static_pointer_cast<model::HighProcessingItem>(item);
            auto work_stealing_queue = WorkQueueManager::getWorkStealingQueue();
            work_stealing_queue.push_back(highProcessingItem);
            std::sort(work_stealing_queue.begin(), work_stealing_queue.end(),
                      [](std::shared_ptr<model::HighProcessingItem> a, std::shared_ptr<model::HighProcessingItem> b) {
                          return a->getDeadline() < b->getDeadline();
                      });

            uint64_t comm_bytes = HIGH_TASK_SIZE;
            auto bytes_per_ms = WorkQueueManager::getBytesPerMillisecond();
            auto comm_time = static_cast<uint64_t>(comm_bytes / bytes_per_ms);
            auto ftp_high_processing_window = currentTime + std::chrono::milliseconds(comm_time + FTP_HIGH_TIME);

            std::vector<std::shared_ptr<model::HighProcessingItem>> newList;

            for (const auto &task: work_stealing_queue) {
                if (ftp_high_processing_window < task->getDeadline())
                    newList.push_back(task);
                else {
                    std::cout << "TASK DEADLINE VIOLATED " << task->getDnnId() << std::endl;
                    web::json::value log;
                    log["dnn"] = web::json::value();
                    log["dnn"]["dnn_id"] = web::json::value::string(task->getDnnId());
                    WorkQueueManager::logManager->add_log(enums::LogTypeEnum::VIOLATED_DEADLINE,
                                                          log);
                }
            }

            WorkQueueManager::setWorkStealingQueue(newList);
        }
        catch (std::exception &e) {
            std::cout << "WorkQueueManager, Add High Comp: Crash: " << e.what() << std::endl;
        }
    }

    int gather_local_capacity_now(std::shared_ptr<model::ComputationDevice> device) {
        auto ct = std::chrono::system_clock::now();

        int cap = 0;

        for (const auto DNN: device->DNNS)
            cap += DNN->getCoreAllocation();

        return cap;
    }

    void WorkQueueManager::high_comp_allocation_call(std::shared_ptr<model::WorkItem> item) {
        try {
            auto workRequest = std::static_pointer_cast<model::WorkRequest>(item);

            int request_counter = workRequest->getRequestCounter();

            std::string sourceHost = (*item->getHostList())[0];
            int device_capacity =
                    MAX_CORE_ALLOWANCE - gather_local_capacity_now(WorkQueueManager::network->devices[sourceHost]);

            auto currentTime = std::chrono::system_clock::now();

            if (device_capacity < FTP_LOW_CORE) {
                std::shared_ptr<model::HighComplexityAllocationComms> high_comp_comms = std::make_shared<model::HighComplexityAllocationComms>(
                        enums::network_comms_types::high_complexity_task_mapping,
                        currentTime,
                        false, sourceHost, request_counter);

                std::shared_ptr<model::BaseNetworkCommsModel> work_request_response = std::static_pointer_cast<model::BaseNetworkCommsModel>(
                        high_comp_comms);


                web::json::value log;
                log["host_request"] = web::json::value(sourceHost);
                log["host_capacity"] = web::json::value::number(device_capacity);
                log["request_counter"] = web::json::value::number(request_counter);
                WorkQueueManager::logManager->add_log(enums::LogTypeEnum::HIGH_COMP_ALLOCATION_FAIL, log);

                services::highTaskAllocation(work_request_response, WorkQueueManager::logManager);
                return;
            }

            auto bytes_per_ms = WorkQueueManager::getBytesPerMillisecond();

            std::shared_ptr<model::HighCompResult> result;

            auto work_queue = WorkQueueManager::getWorkStealingQueue();

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

                WorkQueueManager::off_total[task->getDnnId()] = std::static_pointer_cast<model::BaseCompResult>(task);

                std::shared_ptr<model::HighComplexityAllocationComms> high_comp_comms = std::make_shared<model::HighComplexityAllocationComms>(
                        enums::network_comms_types::high_complexity_task_mapping,
                        currentTime,
                        task,
                        sourceHost, true, request_counter);


                WorkQueueManager::network->devices[task->getAllocatedHost()]->DNNS.push_back(task);

                std::shared_ptr<model::BaseNetworkCommsModel> comm_task = std::static_pointer_cast<model::BaseNetworkCommsModel>(
                        high_comp_comms);

                web::json::value log;
                log["dnn"] = web::json::value(task->convertToJson());
                log["host_request"] = web::json::value(sourceHost);
                log["host_capacity"] = web::json::value::number(device_capacity);
                log["request_counter"] = web::json::value::number(request_counter);
                WorkQueueManager::logManager->add_log(enums::LogTypeEnum::HIGH_COMP_ALLOCATION_SUCCESS, log);
                services::highTaskAllocation(comm_task, WorkQueueManager::logManager);

            } else {
                std::shared_ptr<model::HighComplexityAllocationComms> high_comp_comms = std::make_shared<model::HighComplexityAllocationComms>(
                        enums::network_comms_types::high_complexity_task_mapping,
                        currentTime,
                        false, sourceHost, request_counter);

                std::shared_ptr<model::BaseNetworkCommsModel> work_request_response = std::static_pointer_cast<model::BaseNetworkCommsModel>(
                        high_comp_comms);

                web::json::value log;
                log["host_request"] = web::json::value(sourceHost);
                log["host_capacity"] = web::json::value::number(device_capacity);
                log["request_counter"] = web::json::value::number(request_counter);
                WorkQueueManager::logManager->add_log(enums::LogTypeEnum::HIGH_COMP_ALLOCATION_FAIL, log);

                services::highTaskAllocation(work_request_response, WorkQueueManager::logManager);
            }

            WorkQueueManager::setWorkStealingQueue(work_queue);

        }
        catch (std::exception &e) {
            std::cout << "WorkQueueManager, High Comp Allocation: Crash" << e.what() << std::endl;
        }
    }

    void WorkQueueManager::halt_call(std::shared_ptr<model::WorkItem> workItem) {
        try {
            auto haltWorkItem = std::static_pointer_cast<model::HaltWorkItem>(workItem);
            std::vector<std::string> hostList;
            for (auto [host_id, host]: WorkQueueManager::network->getDevices()) {
                hostList.push_back(host_id);
            }

            auto sourceDeviceId = haltWorkItem->getHostToExamine();

            auto sourceDevice = WorkQueueManager::network->devices[sourceDeviceId];
            auto tasks = sourceDevice->DNNS;

            std::vector<std::pair<int, std::shared_ptr<model::HighCompResult>>> haltCandidates;

            haltCandidates.reserve(tasks.size());

            for (int i = 0; i < tasks.size(); i++) {
                if (tasks[i]->getDnnType() != enums::dnn_type::low_comp)
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
            WorkQueueManager::network->devices[allocated_device]->DNNS.erase(
                    std::remove_if(WorkQueueManager::network->devices[allocated_device]->DNNS.begin(),
                                   WorkQueueManager::network->devices[allocated_device]->DNNS.end(),
                                   [dnn_id](std::shared_ptr<model::BaseCompResult> br) {
                                       return dnn_id == br->getDnnId();
                                   }), WorkQueueManager::network->devices[allocated_device]->DNNS.end());

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

            WorkQueueManager::off_total.erase(dnnToPrune->getDnnId());

            std::cout << "HALTING DNN: " << dnnToPrune->getDnnId() << std::endl;

            std::shared_ptr<model::HaltNetworkCommsModel> baseNetworkCommsModel = std::make_shared<model::HaltNetworkCommsModel>(
                    enums::network_comms_types::halt_req,
                    std::chrono::system_clock::now(), sourceDevice->getHostName(), dnnToPrune->getDnnId(),
                    versionToPrune);

            services::haltReq(std::static_pointer_cast<model::BaseNetworkCommsModel>(baseNetworkCommsModel),
                              WorkQueueManager::logManager);

            /* We attempt to reallocate the DNN if possible */
            auto host_list = std::make_shared<std::vector<std::string>>(
                    std::initializer_list<std::string>{dnnToPrune->getSrcHost()});

            std::shared_ptr<model::HighProcessingItem> highProcessingItem = std::make_shared<model::HighProcessingItem>(
                    host_list,
                    enums::request_type::high_complexity,
                    dnnToPrune->getDeadline(),
                    dnnToPrune->getDnnId());

            WorkQueueManager::add_task(highProcessingItem);
        }
        catch (std::exception &e) {
            std::cout << "WorkQueueManager, Halt: Crash" << e.what() << std::endl;
        }
    }

    void WorkQueueManager::add_task(std::shared_ptr<model::WorkItem> item) {
        try {
            web::json::value log;
            std::unique_lock<std::mutex> lk(WorkQueueManager::work_queue_lock, std::defer_lock);

            lk.lock();
            WorkQueueManager::logManager->add_log(enums::LogTypeEnum::ADD_WORK_TASK, log);
            std::vector<std::shared_ptr<model::WorkItem>> new_work_list;
            new_work_list.push_back(item);

            bool low_task = false;

            for (const auto &work_item: WorkQueueManager::work_queue)
                new_work_list.push_back(work_item);

            int n = static_cast<int>(new_work_list.size());
            for (int i = 0; i < n - 1; ++i) {
                for (int j = 0; j < n - i - 1; ++j) {
                    if (!utils::compare_work_items(new_work_list[j], new_work_list[j + 1])) {
                        std::shared_ptr<model::WorkItem> a = new_work_list[j];
                        new_work_list[j] = new_work_list[j + 1];
                        new_work_list[j + 1] = a;
                    }
                }
            }

            WorkQueueManager::work_queue = new_work_list;

            lk.unlock();
        }
        catch (std::exception e) {
            std::cout << "Failed to add task: \n" << e.what() << "\n" << std::endl;
            exit(1);
        }
    }

    [[noreturn]] void WorkQueueManager::main_loop() {
        try {
            std::unique_lock<std::mutex> lk(WorkQueueManager::work_queue_lock, std::defer_lock);
            std::unique_lock<std::mutex> off_lock(WorkQueueManager::offloaded_lock, std::defer_lock);
            std::unique_lock<std::mutex> net_lock(WorkQueueManager::network_lock, std::defer_lock);
            while (true) {


                if (!WorkQueueManager::work_queue.empty()) {
                    lk.lock();

                    auto current_task = WorkQueueManager::work_queue.front();

                    WorkQueueManager::work_queue.erase(WorkQueueManager::work_queue.begin());

                    lk.unlock();

                    off_lock.lock();
                    net_lock.lock();

                    switch (current_task->getRequestType()) {
                        case enums::request_type::low_complexity:
                            WorkQueueManager::low_comp_allocation_call(current_task);
                        case enums::request_type::high_complexity:
                            WorkQueueManager::add_high_comp_work_item(current_task);
                            break;
                        case enums::request_type::halt_req:
                            WorkQueueManager::halt_call(current_task);
                            break;
                        case enums::request_type::return_task:
                            WorkQueueManager::state_update_call(current_task);
                            break;
                        case enums::request_type::state_update:
                            WorkQueueManager::state_update_call(current_task);
                            break;
                        case enums::request_type::work_request:
                            WorkQueueManager::high_comp_allocation_call(current_task);
                            break;
                    }

                    off_lock.unlock();
                    net_lock.unlock();
                }
            }

        }
        catch (std::exception &e) {
            std::cout << "WORK_QUEUE_MANAGER: something wrong has happened! ;)" << '\n';
            std::cout << e.what() << "\n";
        }
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
                                       ptr)
            : logManager(std::move(ptr)) {
        WorkQueueManager::network = std::make_shared<model::Network>();

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

    void
    highTaskAllocation(const std::shared_ptr<model::BaseNetworkCommsModel> &comm_model,
                       std::shared_ptr<services::LogManager> logManager) {
        auto x = std::thread([comm_model, logManager]() {
            bool failed = true;

            auto highCompComm = std::static_pointer_cast<model::HighComplexityAllocationComms>(comm_model);
            std::string baseURI = "http://" + highCompComm->getHost() + ":" + std::string(HIGH_CLIENT_PORT);

            std::string hostName = highCompComm->getHost();

            json::value output;
            web::json::value log;

            log["request_counter"] = web::json::value::number(highCompComm->getRequestCounter());
            log["comm_time"] = web::json::value::number(
                    std::chrono::duration_cast<std::chrono::milliseconds>(
                            comm_model->getCommTime().time_since_epoch()).count());

            log["success"] = web::json::value::boolean(highCompComm->isSuccess());

            output["success"] = web::json::value::boolean(highCompComm->isSuccess());
            output["request_counter"] = web::json::value::number(highCompComm->getRequestCounter());
            json::value dnn;

            if (highCompComm->isSuccess()) {
                std::shared_ptr<model::HighCompResult> br = highCompComm->getAllocatedTask();
                bool is_source_allocation = br->getSrcHost() == br->getAllocatedHost();

                log["dnn"] = web::json::value(br->convertToJson());
                dnn["source_host"] = web::json::value::string(br->getSrcHost());
                dnn["allocated_host"] = web::json::value::string(
                        is_source_allocation ? "self" : br->getAllocatedHost());
                dnn["start_time"] = web::json::value::number(
                        std::chrono::duration_cast<std::chrono::milliseconds>(
                                br->getEstimatedStart().time_since_epoch()).count());
                dnn["finish_time"] = web::json::value::number(
                        std::chrono::duration_cast<std::chrono::milliseconds>(
                                br->getEstimatedFinish().time_since_epoch()).count());
                dnn["dnn_id"] = web::json::value::string(br->getDnnId());
                dnn["N"] = web::json::value::number(br->getN());
                dnn["M"] = web::json::value::number(br->getM());
                dnn["version"] = web::json::value::number(br->getVersion());
                output["dnn"] = dnn;

            }
            logManager->add_log(enums::LogTypeEnum::OUTBOUND_TASK_ALLOCATION_HIGH, log);

            int counter = 0;

            bool continue_process = false;
            bool *continue_process_ptr = &continue_process;
            bool *failed_ptr = &failed;

            auto time_since_epoch = std::chrono::system_clock::now().time_since_epoch().count();
            output["request_version"] = web::json::value::string(std::to_string(time_since_epoch));
            while (failed) {
                failed = true;
                continue_process = false;

                std::chrono::time_point<std::chrono::system_clock> comm_start = std::chrono::system_clock::now();

                std::cout << "HIGH_COMP_ALLOCATION: " << baseURI << "/" << std::string(HIGH_TASK_ALLOCATION)
                          << " COUNTER - " << highCompComm->getRequestCounter()
                          << " ATTEMPT: " << counter << std::endl;

                counter++;

                std::string success_str =
                        "HIGH_COMP_ALLOCATION: " + baseURI + "/" + std::string(HIGH_TASK_ALLOCATION) + " COUNTER - "
                        + std::to_string(highCompComm->getRequestCounter()) + " SUCCESS!";
                try {

                    failed = false;
                    http::client::http_client(baseURI).request(
                            http::methods::POST,
                            "/" +
                            std::string(HIGH_TASK_ALLOCATION),
                            output.serialize()).then(
                            [continue_process_ptr, success_str, failed_ptr](const web::http::http_response res) {
                                if (res.status_code() != http::status_codes::OK)
                                    *failed_ptr = true;
                                else
                                    std::cout << success_str << std::endl;
                                *continue_process_ptr = true;

                            }).wait();

                    while (!continue_process);

                }
                catch (const std::exception &e) {
                    std::cout << "NETWORK_QUEUE_MANAGER: HIGH TASK SYSTEM_CRASH" << std::endl;
                    failed = true;
                    std::cout << e.what() << std::endl;
                }
                catch (const http::http_exception &e) {
                    std::cout << "NETWORK_QUEUE_MANAGER: HIGH TASK SYSTEM_CRASH" << std::endl;
                    failed = true;
                    std::cout << e.what() << std::endl;
                }
                // Sleep for 100 milliseconds
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        });
        x.detach();
    }

    void haltReq(std::shared_ptr<model::BaseNetworkCommsModel> comm_model,
                 std::shared_ptr<services::LogManager> logManager) {
        auto x = std::thread([comm_model, logManager]() {
            bool failed = true;
            bool *failed_ptr = &failed;

            auto haltCommModel = static_pointer_cast<model::HaltNetworkCommsModel>(comm_model);

            web::json::value result;

            web::json::value log;
            log["comm_time"] = web::json::value::number(std::chrono::duration_cast<std::chrono::milliseconds>(
                    comm_model->getCommTime().time_since_epoch()).count() * 1000);
            logManager->add_log(enums::LogTypeEnum::OUTBOUND_HALT_REQUEST, log);

            result["dnn_id"] = web::json::value::string(haltCommModel->getDnnId());
            result["version"] = web::json::value::number(haltCommModel->getVersionNumber());

            bool continue_proccess = false;
            bool *continue_process_ptr = &continue_proccess;

            auto time_since_epoch = std::chrono::system_clock::now().time_since_epoch().count();
            result["request_version"] = web::json::value::string(std::to_string(time_since_epoch));
            while (failed) {
                try {
                    failed = false;
                    web::http::client::http_client(
                            "http://" + haltCommModel->getHostToContact() + ":" +
                            std::string(HIGH_CLIENT_PORT)).request(
                            web::http::methods::POST,
                            "/" +
                            std::string(HALT_ENDPOINT),
                            result.serialize()).then(
                            [continue_process_ptr, failed_ptr](web::http::http_response response) {
                                if (response.status_code() != http::status_codes::OK)
                                    *failed_ptr = true;
                                else
                                    std::cout << "HALT REQ: SUCCESS" << std::endl;
                                *continue_process_ptr = true;
                            }).wait();

                    while (!continue_proccess);
                }
                catch (const std::exception &e) {
                    std::cout << "NETWORK_QUEUE_MANAGER: Halt Error exception: " << e.what() << std::endl;
                }
                catch (const http::http_exception &e) {
                    std::cout << "NETWORK_QUEUE_MANAGER: Halt Error exception: " << e.what() << std::endl;
                }
                // Sleep for 100 milliseconds
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        });

        x.detach();
    }

    void
    lowTaskAllocation(std::shared_ptr<model::BaseNetworkCommsModel> comm_model,
                      std::shared_ptr<services::LogManager> logManager) {

        auto x = std::thread([comm_model, logManager]() {
            bool failed = true;

            // Create a pointer to a bool
            bool *failed_ptr = &failed;
            auto lowCompComm = static_pointer_cast<model::LowComplexityAllocationComms>(comm_model);
            auto task_res = lowCompComm->getAllocatedTask();
            std::string host = lowCompComm->getHost();

            json::value result;
            result["dnn_id"] = json::value::string(task_res->getDnnId());
            result["start_time"] = json::value::number(std::chrono::duration_cast<std::chrono::milliseconds>(
                    task_res->getEstimatedStart().time_since_epoch()).count());
            result["finish_time"] = json::value::number(std::chrono::duration_cast<std::chrono::milliseconds>(
                    task_res->getEstimatedFinish().time_since_epoch()).count());

            web::json::value log;
            log["dnn"] = web::json::value(task_res->convertToJson());
            log["comm_time"] = web::json::value::number(std::chrono::duration_cast<std::chrono::milliseconds>(
                    comm_model->getCommTime().time_since_epoch()).count());
            log["estimated_start"] = web::json::value::string(
                    utils::debugTimePointToString(task_res->getEstimatedStart()));
            log["estimated_finish"] = web::json::value::string(
                    utils::debugTimePointToString(task_res->getEstimatedFinish()));
            logManager->add_log(enums::LogTypeEnum::OUTBOUND_LOW_COMP_ALLOCATION, log);


            bool continue_process = false;
            bool *continue_process_ptr = &continue_process;
            int counter = 0;

            auto time_since_epoch = std::chrono::system_clock::now().time_since_epoch().count();


            auto high_task_item = web::json::value();
            high_task_item["request_version"] = web::json::value::string(std::to_string(time_since_epoch));
            high_task_item["low_comp_id"] = web::json::value::string(task_res->getDnnId());

            auto success_str = std::string("LOW_COMP_ALLOCATION: ") + host + std::string("/") + std::string(LOW_TASK_ALLOCATION)
                                         + std::string(" ATTEMPT: ");

            while (failed) {
                try {
                    std::cout << "LOW_COMP_ALLOCATION: " << host << "/" << std::string(LOW_TASK_ALLOCATION)
                              << " ATTEMPT: " << counter << std::endl;



                    counter++;
                    failed = false;
                    web::http::client::http_client("http://" + host + ":" + std::string(LOW_COMP_PORT)).request(
                            web::http::methods::POST,
                            "/" + std::string(LOW_TASK_ALLOCATION), result.serialize()).then(
                            [continue_process_ptr, failed_ptr, success_str, counter](const web::http::http_response & response) {
                                *continue_process_ptr = true;

                                if (response.status_code() != http::status_codes::OK)
                                    *failed_ptr = true;
                                else
                                    std::cout << success_str << counter << std::endl;
                                *continue_process_ptr = true;
                            }).wait();

                    while (!continue_process);
                }
                catch (const std::exception &e) {
                    std::cout << "NETWORK_QUEUE_MANAGER: LOW_COMP SYSTEM CRASH" << std::endl;
                }
                catch (const http::http_exception &e) {
                    std::cout << "NETWORK_QUEUE_MANAGER: LOW COMP SYSTEM_CRASH" << std::endl;
                }
                // Sleep for 100 milliseconds
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        });

        x.detach();

    }

} // services