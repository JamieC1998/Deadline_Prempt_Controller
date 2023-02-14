//
// Created by jamiec on 9/23/22.
//

#include <thread>
#include <utility>
#include "WorkQueueManager.h"
#include "../../model/data_models/WorkItems/DAGDisruptItem/DAGDisruption.h"
#include "../../model/data_models/WorkItems/ProcessingItem/HighProcessingItem/HighProcessingItem.h"
#include "../LowCompServices/LowCompServices.h"
#include "../../model/data_models/WorkItems/StateUpdate/StateUpdate.h"
#include "../NetworkServices/NetworkServices.h"
#include "../../utils/UtilFunctions/UtilFunctions.h"
#include "../../Constants/CLIENT_DETAILS.h"
#include "../HighCompServices/HighCompServices.h"
#include "../../Constants/AllocationMacros.h"
#include "../../model/data_models/NetworkCommsModels/HighComplexityAllocation/HighComplexityAllocationComms.h"
#include "../../model/data_models/WorkItems/ProcessingItem/LowProcessingItem/LowProcessingItem.h"
#include "../../model/data_models/NetworkCommsModels/LowComplexityAllocation/LowComplexityAllocationComms.h"
#include "../../model/data_models/WorkItems/PruneItem/PruneItem.h"
#include "../../model/data_models/NetworkCommsModels/OutboundUpdate/OutboundUpdate.h"
#include "../../model/data_models/NetworkCommsModels/HaltNetworkCommsModel/HaltNetworkCommsModel.h"

namespace services {
    std::atomic<int> WorkQueueManager::thread_counter = 0;

    void state_update_call(std::shared_ptr<model::WorkItem> item, std::shared_ptr<WorkQueueManager> queueManager) {
        std::shared_ptr<model::StateUpdate> stateUpdate = std::static_pointer_cast<model::StateUpdate>(item);

        std::unique_lock<std::mutex> off_lock(queueManager->offloaded_lock, std::defer_lock);
        off_lock.lock();

        auto dnn = queueManager->off_high[stateUpdate->getDnnId()];

        for (auto [partition_id, finish_time]: stateUpdate->getFinishTimes()) {
            dnn->tasks[stateUpdate->getConvidx()]->partitioned_tasks[partition_id]->setActualFinish(finish_time);
            dnn->tasks[stateUpdate->getConvidx()]->partitioned_tasks[partition_id]->setCompleted(true);

            /* TODO LOG UPDATE */
        }

        dnn->tasks[stateUpdate->getConvidx()]->setCompleted(true);

        bool completed = true;
        for(auto [convidx, block]: dnn->tasks){
            if(!block->isCompleted()) {
                completed = false;
                break;
            }
        }

        if(completed){
            web::json::value log;
            log["dnn_details"] = dnn->convertToJson();
            queueManager->logManager->add_log(enums::LogTypeEnum::HIGH_COMP_FINISH, log);

            auto pruneItem = std::make_shared<model::PruneItem>(enums::request_type::prune_dnn,
                                                                dnn->getDnnId(),
                                                                stateUpdate->getConvidx());
            queueManager->add_task(pruneItem);
        }

        off_lock.unlock();
        services::WorkQueueManager::decrementThreadCounter();
    }

    void prune_dnn_call(std::shared_ptr<model::WorkItem> item, std::shared_ptr<WorkQueueManager> queueManager) {
        auto prune_item = std::static_pointer_cast<model::PruneItem>(item);

        /* TODO PRUNING DNN LOG HERE */
        /* Fetch the DNN details */
        std::string dnn_id = prune_item->getDnnId();
        std::string info_forward = prune_item->getNextConvBlock();

        web::json::value log;
        log["dnn_id"] = web::json::value::string(dnn_id);
        queueManager->logManager->add_log(enums::LogTypeEnum::DNN_PRUNE, log);

        auto dnn = queueManager->off_high[dnn_id];

        /* Remove the DNN from the DAG queues */
        std::unique_lock<std::mutex> off_lock(queueManager->offloaded_lock, std::defer_lock);
        off_lock.lock();
        queueManager->off_total.erase(dnn_id);
        queueManager->off_high.erase(dnn_id);
        off_lock.unlock();

        /* Begin iterating through the DNN to gather tasks and links to remove */
        std::vector<std::shared_ptr<model::Task>> tasks_to_prune;
        std::vector<std::shared_ptr<model::LinkAct>> links_to_prune;

        links_to_prune.push_back(dnn->getUploadData());
        dnn->resetUploadData();

        for (auto [convidx, conv_block]: dnn->tasks) {
            for (auto [partition_id, link]: conv_block->assembly_upload_windows) {
                links_to_prune.push_back(link);
                link.reset();
            }
            conv_block->assembly_upload_windows.clear();
            links_to_prune.push_back(conv_block->state_update);
            conv_block->state_update.reset();

            for (auto [partition_id, partition]: conv_block->partitioned_tasks) {
                links_to_prune.push_back(partition->getInputData());
                tasks_to_prune.push_back(partition);
            }
            conv_block->assembly_upload_windows.clear();
        }

        std::unique_lock<std::mutex> network_lock(queueManager->network_lock, std::defer_lock);
        network_lock.lock();

        for (const auto &task: tasks_to_prune) {
            auto device = queueManager->network->getDevices()[task->getAllocatedHost()];

            auto it = device->TASKS.begin();
            for (int i = 0; i < device->TASKS.size(); i++) {
                if (device->getTasks()[i]->getUniqueTaskId() == task->getUniqueTaskId()) {
                    device->TASKS.erase(it + i);
                    break;
                }
            }
        }

        for (const auto &prune_link: links_to_prune) {
            auto it = queueManager->network->network_link.begin();
            for (int i = 0; i < queueManager->network->network_link.size(); i++) {
                if (queueManager->network->network_link[i]->getLinkActivityId() == prune_link->getLinkActivityId()) {
                    queueManager->network->network_link.erase(it + i);
                }
            }
        }
        network_lock.unlock();
        dnn->tasks.clear();

        services::WorkQueueManager::decrementThreadCounter();
    }

    /*Function receives alow complexity DNN allocation request*/
    void low_comp_allocation_call(std::shared_ptr<model::WorkItem> item, std::shared_ptr<WorkQueueManager> queueManager) {
        auto proc_item = std::static_pointer_cast<model::LowProcessingItem>(item);

        std::unique_lock<std::mutex> net_lock(queueManager->network_lock);
        std::map<std::string, std::shared_ptr<model::ComputationDevice>> devices = queueManager->network->getDevices();

        /*Need to create a copy of the network list so that we can keep track of incomplete net allocations */
        std::shared_ptr<std::vector<std::shared_ptr<model::LinkAct>>> copyList;
        std::copy(queueManager->network->getLink().begin(), queueManager->network->getLink().end(), copyList->begin());
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
                currentTime, bw_bytes, data_size_bytes, copyList);
        net_lock.unlock();
        std::shared_ptr<model::LinkAct> state_transfer = std::make_shared<model::LinkAct>(
                std::make_pair(times->first, times->second));
        state_transfer->setHostNames(std::make_pair(CONTROLLER_HOSTNAME, host));
        state_transfer->setDataSize(data_size_bytes);
        state_transfer->setIsMeta(false);
        copyList->push_back(state_transfer);

        std::sort(copyList->begin(), copyList->end(),
                  [](const std::shared_ptr<model::LinkAct> &a, const std::shared_ptr<model::LinkAct> &b) {
                      return a->getStartFinTime().second < b->getStartFinTime().second;
                  });


        /* Using the input upload window for each task generated we now allocate a time slot on their respective hosts */
        auto result = services::allocate_task(item, devices, times);
        auto time_window = result.second;
        /* If we cannot allocate a device for even one task we instead create a new allocation request and a halt request */
        if (!result.first) {

            auto workItem = std::make_shared<model::WorkItem>(enums::request_type::halt_req);
            queueManager->add_task(workItem);
            auto newTask = std::make_shared<model::LowProcessingItem>(item->getHostList(),
                                                                      enums::request_type::low_complexity,
                                                                      proc_item->getDeadline(),
                                                                      proc_item->getDnnIdAndDevice());
            queueManager->add_task(std::static_pointer_cast<model::WorkItem>(newTask));

            web::json::value log;
            log["dnn_id"] = web::json::value::string(newTask->getDnnIdAndDevice().first);
            queueManager->logManager->add_log(enums::LogTypeEnum::LOW_COMP_ALLOCATION_FAIL, log);
            web::json::value halt_log;
            halt_log["dnn_id"] = web::json::value::string(newTask->getDnnIdAndDevice().first);
            queueManager->logManager->add_log(enums::LogTypeEnum::HALT_REQUEST, log);
            services::WorkQueueManager::decrementThreadCounter();
            return;

        } else {
            /* For each allocated low comp task we create a Result object
             * the key is the dnn_id*/

            auto deadlines = proc_item->getDeadline();
            auto bR = std::make_shared<model::LowCompResult>(dnn_id, enums::dnn_type::low_comp, host,
                                                             deadlines, state_transfer);

            std::shared_ptr<model::Task> task = std::make_shared<model::Task>(dnn_id, enums::dnn_type::low_comp,
                                                                              time_window->first, time_window->second,
                                                                              host,
                                                                              state_transfer);

            bR->setTask(task);
            bR->setEstimatedStart(bR->getUploadData()->getStartFinTime().first);
            bR->setEstimatedFinish(task->getEstimatedFinish());
            std::unique_lock<std::mutex> offload_lock(queueManager->offloaded_lock, std::defer_lock);
            offload_lock.lock();

            auto base_cast_result = std::static_pointer_cast<model::BaseCompResult>(bR);
            queueManager->off_low[base_cast_result->getDnnId()] = bR;
            queueManager->off_total[base_cast_result->getDnnId()] = base_cast_result;
            offload_lock.unlock();

            /* Add communication times to the network link */
            net_lock.lock();
            queueManager->network->addComm(state_transfer);

            std::shared_ptr<model::LowComplexityAllocationComms> baseNetworkCommsModel =
                    std::make_shared<model::LowComplexityAllocationComms>(
                            enums::network_comms_types::low_complexity_allocation,
                            bR->getEstimatedStart(),
                            bR,
                            bR->getSrcHost());

            queueManager->network->getDevices()[bR->getSrcHost()]->TASKS.push_back(
                    bR->getTask());
            queueManager->networkQueueManager->addTask(baseNetworkCommsModel);
            net_lock.unlock();
            web::json::value log;
            log["dnn_details"] = bR->convertToJson();
            queueManager->logManager->add_log(enums::LogTypeEnum::LOW_COMP_ALLOCATION_SUCCESS, log);
        }
        services::WorkQueueManager::decrementThreadCounter();
    }


    void high_comp_allocation_call(std::shared_ptr<model::WorkItem> item, std::shared_ptr<WorkQueueManager> queueManager) {
        auto processingItem = std::static_pointer_cast<model::HighProcessingItem>(item);
        bool isReallocation = processingItem->isReallocation();

        std::string sourceHost = (*item->getHostList())[0];

        std::shared_ptr<model::HighCompResult> baseResult;

        std::shared_ptr<model::FTP_Lookup> ftp_lookup_table = queueManager->lookup_table;

        uint64_t state_data_size_bytes = queueManager->state_size;

        auto bytes_per_ms = queueManager->getBytesPerMillisecond();

        std::chrono::time_point<std::chrono::system_clock> currentTime = std::chrono::system_clock::now();
        //Need a copy of the network link so we dont mutate it in event of failed allocation
        std::shared_ptr<std::vector<std::shared_ptr<model::LinkAct>>> safe_network_link;

        //Will hold the actual link slots so we copy in the entire copied network link;
        std::vector<std::shared_ptr<model::LinkAct>> result_link_slots;


        std::unique_lock<std::mutex> netlock(queueManager->network_lock, std::defer_lock);
        netlock.lock();


        //Copying in the network link to our safe mutable copy
        std::copy(queueManager->network->getLink().begin(), queueManager->network->getLink().end(),
                  safe_network_link->begin());

        //Will hold our resultant task map
        std::map<std::string, std::shared_ptr<model::ResultBlock>> partition_allocations_per_block;

        if (isReallocation) {
            baseResult = processingItem->getBaseResult();
            baseResult->setVersion(std::chrono::system_clock::now().time_since_epoch().count() * 1000);
            partition_allocations_per_block.insert(baseResult->tasks.begin(), baseResult->tasks.end());
        }

        /* We need a copy of the task list of each device so that we aren't modifying them as we allocate tasks,
         * this way if we fail to allocate the DNN further down the chain, we don't need to go back and prune tasks */
        std::map<int, std::vector<std::shared_ptr<model::Task>>> temp_device_task_list;
        for (auto [device_id, device]: queueManager->network->getDevices()) {
            std::vector<std::shared_ptr<model::Task>> tmp_task;
            std::copy(device->getTasks().begin(), device->getTasks().end(), tmp_task.begin());
            temp_device_task_list[device->getId()] = tmp_task;
        }

        netlock.unlock();

        int j = (!isReallocation) ? 1 : baseResult->getLastCompleteConvIdx() + 1;
        /* This is where we store "successful task allocations" as we iterate through cores
         * key is the partition block id (ie. its N * M), the value is the task */
        std::shared_ptr<model::ResultBlock> temp_partition_allocations_core;

        /* This is how we keep track of current allocations to devices inside core iterations */
        std::map<int, std::vector<std::shared_ptr<model::Task>>> temporary_device_task_lists_core;

        //NEED TO FIND EARLIEST LINK SLOT TO COMMUNICATE DATA TO SOURCE DEVICE
        auto initial_data_communication_slot = services::findLinkSlot(currentTime, bytes_per_ms,
                                                                      queueManager->state_size, safe_network_link);
        /* We need to create a link activity for the initial upload of data */
        std::shared_ptr<model::LinkAct> initial_assembly_data = std::make_shared<model::LinkAct>();
        initial_assembly_data->setIsMeta(false);
        initial_assembly_data->setDataSize(state_data_size_bytes);
        initial_assembly_data->setStartFinTime(
                std::make_pair(initial_data_communication_slot->first, initial_data_communication_slot->second));
        initial_assembly_data->setHostNames(std::make_pair(CONTROLLER_HOSTNAME, sourceHost));

        std::string starting_convidx = std::to_string(j);

        if (!isReallocation)
            baseResult = std::make_shared<model::HighCompResult>(
                    processingItem->getDnnId(),
                    enums::dnn_type::high_comp,
                    sourceHost,
                    processingItem->getDeadline(),
                    initial_data_communication_slot->second,
                    starting_convidx,
                    initial_assembly_data);
        else {
            baseResult->setUploadData(initial_assembly_data);
            baseResult->setEstimatedStart(initial_data_communication_slot->second);
        }

        for (auto [convidx, convolutional_block]: ftp_lookup_table->partition_setup) {
            if (std::stoi(convidx) < std::stoi(starting_convidx))
                continue;

            bool allocation_found = false;
            std::vector<int> core_config;
            for (auto [key, value]: convolutional_block){
                int core_key = std::stoi(key);
                if (core_key <= MAX_CORE_ALLOWANCE){
                    core_config.push_back(core_key);
                }
            }
            std::sort(core_config.begin(), core_config.end(), [](int a, int b) {
                return a > b;
            });

            for (int current_core: core_config) {
                int CURRENT_N = convolutional_block[std::to_string(current_core)].getN();
                int CURRENT_M = convolutional_block[std::to_string(current_core)].getM();

                /*We need to clear these between core iterations to
                 * discard the previous core iteration data that failed */
                temporary_device_task_lists_core.clear();
                temp_partition_allocations_core = std::make_shared<model::ResultBlock>(CURRENT_N, CURRENT_M);

                //Refer to above comment
                //NEED to double check as it seems like this is copying from the device list, which will ignore previous allocations
                for (auto [device_id, device]: temp_device_task_list) {
                    std::vector<std::shared_ptr<model::Task>> tmp_task;
                    std::copy(device.begin(), device.end(), tmp_task.begin());
                    temporary_device_task_lists_core[device_id] = tmp_task;
                }

                /*Inside our core iteration we need a copy of the network link, so that the previous
                 * core iterations failed data isnt affecting the allocation */
                std::shared_ptr<std::vector<std::shared_ptr<model::LinkAct>>> core_it_safe_network_link;
                std::copy(safe_network_link->begin(), safe_network_link->end(),
                          core_it_safe_network_link->begin());

                //Will hold the actual link slots so we dont have core iterations affect our result links;
                std::vector<std::shared_ptr<model::LinkAct>> core_iteration_result_link_slots;
                //Iterate through each of the cores to find a valid allocation
                for (int i = 0; i < current_core; i++) {
                    auto current_config_details = ftp_lookup_table->partition_setup[convidx][std::to_string(
                            current_core)];
                    auto [part_N, part_M] = utils::fetchN_M(current_core, CURRENT_N, CURRENT_M);
                    uint64_t upload_data_size =
                            state_data_size_bytes + (current_config_details.getTileSize()[current_core]);

                    //If it's the first iteration we use the starting time as our base
                    auto task_base_time = (convidx == starting_convidx) ? baseResult->getEstimatedStart()
                                                                        : partition_allocations_per_block[std::to_string(
                                    j -
                                    1)]->getAssemblyFinTime();
                    auto time_slot = services::findLinkSlot(
                            task_base_time, bytes_per_ms,
                            upload_data_size, core_it_safe_network_link);

                    std::shared_ptr<model::LinkAct> input_data = std::make_shared<model::LinkAct>();
                    input_data->setIsMeta(false);
                    input_data->setDataSize(upload_data_size);
                    input_data->setStartFinTime(std::make_pair(time_slot->first, time_slot->second));

                    std::chrono::time_point<std::chrono::system_clock> finish_time = time_slot->second +
                                                                                     std::chrono::milliseconds{
                                                                                             current_config_details.getProcTimeMilliseconds()};
                    auto selected_device = services::findNode(queueManager->network->getDevices(),
                                                              temporary_device_task_lists_core,
                                                              time_slot->second, finish_time);

                    if (selected_device.first != TASK_NOT_FOUND) {
                        std::shared_ptr<model::Task> potential_task = std::make_shared<model::Task>();
                        potential_task->setEstimatedStart(time_slot->second);
                        potential_task->setEstimatedFinish(finish_time);
                        potential_task->setPreviousConv(j - 1);
                        potential_task->setPartitionBlockId(i);
                        potential_task->setAllocatedHost(selected_device.second);
                        potential_task->setTaskOutputSizeBytes(upload_data_size);
                        potential_task->setN(part_N);
                        potential_task->setM(part_M);

                        temporary_device_task_lists_core[selected_device.first].push_back(potential_task);
                        temp_partition_allocations_core->partitioned_tasks[i] = potential_task;

                        input_data->setHostNames(
                                std::make_pair((convidx == starting_convidx) ? CONTROLLER_HOSTNAME
                                                                             : partition_allocations_per_block[std::to_string(
                                                               j -
                                                               1)]->getAssemblyHost(),
                                               selected_device.second));
                        core_it_safe_network_link->push_back(input_data);
                        core_iteration_result_link_slots.push_back(input_data);

                        potential_task->setInputData(input_data);
                    }

                        /* If partition fails we break and exit to the next core configuration */
                    else {
                        web::json::value log;
                        log["dnn_id"] = web::json::value::string(processingItem->getDnnId());
                        log["convidx"] = web::json::value::string(convidx);
                        log["current_core_config"] = web::json::value::number(current_core);
                        log["failed_partition"] = web::json::value::number(i);
                        queueManager->logManager->add_log(enums::LogTypeEnum::HIGH_COMP_ALLOCATION_CORE_FAIL, log);
                        break;
                    }
                }

                /* If we have allocated every task in the partition config
                 * we save the result block to our results table */
                if (temp_partition_allocations_core->partitioned_tasks.size() == current_core) {

                    /* NEED TO TRANSFER THE RESULTS OF EACH PARTITION TO A SINGLE POINT TO ENSURE
                     * THAT ASSEMBLY CAN OCCUR */
                    std::vector<std::shared_ptr<model::LinkAct>> assembly_tasks;
                    std::string assembly_host = temp_partition_allocations_core->partitioned_tasks[0]->getAllocatedHost();

                    std::chrono::time_point<std::chrono::system_clock> largest_assembly_upload_finish_time = std::chrono::system_clock::now();
                    for (auto [task_id, task]: temp_partition_allocations_core->partitioned_tasks) {
                        auto assembly_upload_time_slot = services::findLinkSlot(
                                task->getEstimatedFinish(), bytes_per_ms,
                                task->getTaskOutputSizeBytes(), core_it_safe_network_link);

                        auto output_upload = std::make_shared<model::LinkAct>();
                        output_upload->setIsMeta(false);
                        output_upload->setHostNames(std::make_pair(task->getAllocatedHost(), assembly_host));
                        output_upload->setDataSize(task->getTaskOutputSizeBytes());
                        output_upload->setStartFinTime(
                                std::make_pair(assembly_upload_time_slot->first, assembly_upload_time_slot->second));

                        temp_partition_allocations_core->assembly_upload_windows[task_id] = output_upload;

                        core_it_safe_network_link->push_back(output_upload);
                        core_iteration_result_link_slots.push_back(output_upload);

                        if (largest_assembly_upload_finish_time < assembly_upload_time_slot->second)
                            largest_assembly_upload_finish_time = assembly_upload_time_slot->second;

                    }

                    auto state_update_time_slot = services::findLinkSlot(
                            largest_assembly_upload_finish_time, bytes_per_ms,
                            state_data_size_bytes, core_it_safe_network_link);

                    std::shared_ptr<model::LinkAct> state_update;
                    state_update->setIsMeta(true);
                    state_update->setHostNames(std::make_pair(assembly_host, CONTROLLER_HOSTNAME));
                    state_update->setDataSize(state_data_size_bytes);
                    state_update->setStartFinTime(
                            std::make_pair(state_update_time_slot->first, state_update_time_slot->second));

                    temp_partition_allocations_core->state_update = state_update;

                    /* Need to set the window for assembly */
                    std::chrono::time_point<std::chrono::system_clock> assembly_start = std::chrono::system_clock::now();

                    for (auto [key, upload_window]: temp_partition_allocations_core->assembly_upload_windows)
                        if (upload_window->getStartFinTime().first > assembly_start)
                            assembly_start = upload_window->getStartFinTime().first;

                    temp_partition_allocations_core->setAssemblyStartTime(assembly_start);
                    temp_partition_allocations_core->setAssemblyFinTime(
                            largest_assembly_upload_finish_time + std::chrono::milliseconds{ASSEMBLY_PADDING});
                    temp_partition_allocations_core->setAssemblyHost(assembly_host);
                    temp_partition_allocations_core->setStateUpdateFinTime(state_update_time_slot->second);

                    core_it_safe_network_link->push_back(state_update);
                    core_iteration_result_link_slots.push_back(state_update);

                    partition_allocations_per_block[convidx] = temp_partition_allocations_core;
                    temp_device_task_list = temporary_device_task_lists_core;
                    result_link_slots = core_iteration_result_link_slots;

                    safe_network_link->clear();
                    std::copy(core_it_safe_network_link->begin(), core_it_safe_network_link->end(),
                              safe_network_link->begin());

                    allocation_found = true;
                    break;
                }
            }
            if (!allocation_found) {
                web::json::value log;
                log["dnn_id"] = web::json::value::string(processingItem->getDnnId());
                log["reason"] = web::json::value::string("capacity");
                queueManager->logManager->add_log((isReallocation) ? enums::LogTypeEnum::HIGH_COMP_REALLOCATION_FAIL : enums::LogTypeEnum::HIGH_COMP_ALLOCATION_FAIL, log);
                services::WorkQueueManager::decrementThreadCounter();
                return;
            }

            j++;
        }

        int last_block = static_cast<int>(partition_allocations_per_block.size());
        baseResult->setEstimatedFinish(
                partition_allocations_per_block[std::to_string(last_block)]->getStateUpdateFinTime());

        /* If we still cannot satisfy the deadline with the greediest approach
         * exit */
        if(baseResult->getEstimatedFinish() > processingItem->getDeadline()){
            web::json::value log;
            log["dnn_id"] = web::json::value::string(processingItem->getDnnId());
            log["reason"] = web::json::value::string("deadline");
            queueManager->logManager->add_log((isReallocation) ? enums::LogTypeEnum::HIGH_COMP_REALLOCATION_FAIL : enums::LogTypeEnum::HIGH_COMP_ALLOCATION_FAIL, log);
            services::WorkQueueManager::decrementThreadCounter();
            return;
        }

        netlock.lock();
        /* NEED TO ADD TASKS TO DEVICES AND COMMS TO THE LINK */
        queueManager->network->addComms(result_link_slots);
        for (auto [dev_id, task_list]: temp_device_task_list) {
            std::string host_name = task_list[0]->getAllocatedHost();
            queueManager->network->getDevices()[host_name]->setTasks(task_list);
        }
        netlock.unlock();
        //NEED TO ONLY UPLOAD FIRST TASK
        baseResult->tasks = partition_allocations_per_block;

        std::unique_lock<std::mutex> offload_lock(queueManager->offloaded_lock, std::defer_lock);
        offload_lock.lock();
        std::string dnn_id = std::static_pointer_cast<model::BaseCompResult>(baseResult)->getDnnId();
        queueManager->off_high[dnn_id] = baseResult;
        queueManager->off_total[dnn_id] = std::static_pointer_cast<model::BaseCompResult>(baseResult);
        offload_lock.unlock();

        std::shared_ptr<model::BaseNetworkCommsModel> comm_task;

        std::shared_ptr<model::HighComplexityAllocationComms> high_comp_comms = std::make_shared<model::HighComplexityAllocationComms>(
                enums::network_comms_types::high_complexity_task_mapping,
                baseResult->getUploadData()->getStartFinTime().first,
                baseResult,
                baseResult->getSrcHost());

        comm_task = std::static_pointer_cast<model::BaseNetworkCommsModel>(
                high_comp_comms);

        web::json::value log;
        log["dnn"] = web::json::value(baseResult->convertToJson());
        queueManager->logManager->add_log((isReallocation) ? enums::LogTypeEnum::HIGH_COMP_REALLOCATION_SUCCESS : enums::LogTypeEnum::HIGH_COMP_ALLOCATION_SUCCESS, log);


        queueManager->networkQueueManager->addTask(comm_task);

        services::WorkQueueManager::decrementThreadCounter();
    }

    void dag_disruption_call(std::shared_ptr<model::WorkItem> item, std::shared_ptr<WorkQueueManager> queueManager) {
        auto dag_item = std::static_pointer_cast<model::DAGDisruption>(item);

        //{parition_model_id: The unique DNN ID, convidx: the block group the partition belongs to, initial_dnn_id: the id assigned the partitioned block}
        int initial_partition_id = dag_item->getPartitionId();
        std::string initial_condidx = dag_item->getConvidx();
        std::string initial_dnn_id = dag_item->getPartitionedDnnId();

        std::unique_lock<std::mutex> offload_lock(queueManager->offloaded_lock, std::defer_lock);
        offload_lock.lock();

        /* If our DNN no longer exists (Possibly due to already being pruned, moments before
         * we exit */
        if (!queueManager->off_total.count(initial_dnn_id)) {
            services::WorkQueueManager::decrementThreadCounter();
            offload_lock.unlock();
            return;
        }

        std::shared_ptr<model::HighCompResult> violated_dnn = std::static_pointer_cast<model::HighCompResult>(
                queueManager->off_total[initial_dnn_id]);
        auto initial_conv_block = violated_dnn->tasks[initial_condidx];
        auto initial_task = initial_conv_block->partitioned_tasks[initial_partition_id];
        initial_task->setActualFinish(
                dag_item->getFinishTime());
        int initial_unique_task_id = initial_task->getUniqueTaskId();

        /* Need to iterate through the task_list of the device it was allocated to, to make sure that a given task
         * does not impact four others, if it does exit early */
        std::unique_lock<std::mutex> net_lock(queueManager->network_lock, std::defer_lock);
        net_lock.lock();

        int task_counter;
        for (const auto &task: queueManager->network->getDevices()[initial_task->getAllocatedHost()]->TASKS) {
            if (task->getUniqueTaskId() != initial_unique_task_id &&
                task->getEstimatedStart() >= initial_task->getEstimatedFinish() &&
                task->getEstimatedFinish() <= task->getActualFinish())
                task_counter++;
        }

        if (task_counter == 4) {
            int initial_convidx_int = std::stoi(initial_condidx);
            std::string conv_block_to_update_to = (initial_convidx_int == violated_dnn->tasks.size()) ? initial_condidx
                                                                                                      : std::to_string(
                            initial_convidx_int + 1);
            auto pruneItem = std::make_shared<model::PruneItem>(enums::request_type::prune_dnn,
                                                                violated_dnn->getDnnId(),
                                                                conv_block_to_update_to);

            web::json::value log;
            log["dnn_id"] = web::json::value(violated_dnn->getDnnId());
            queueManager->logManager->add_log(enums::LogTypeEnum::DAG_DISRUPTION_FAIL, log);


            queueManager->add_task(std::static_pointer_cast<model::WorkItem>(pruneItem));
            services::WorkQueueManager::decrementThreadCounter();
            net_lock.unlock();
            offload_lock.unlock();
            return;

        }

        auto initial_assembly_upload_start_and_finish_time = initial_conv_block->assembly_upload_windows[initial_partition_id]->getStartFinTime();
        /* Need to check to see the violated tasks new finish time is greater than its upload start,
         * if so we need to now update the upload windows, if not then we can end the violation trace here */
        if (initial_task->getActualFinish() >= initial_assembly_upload_start_and_finish_time.first) {
            auto violation_amount =
                    initial_task->getActualFinish() - initial_assembly_upload_start_and_finish_time.first +
                    std::chrono::milliseconds{1};
            initial_assembly_upload_start_and_finish_time.first =
                    initial_assembly_upload_start_and_finish_time.first + violation_amount;
            initial_assembly_upload_start_and_finish_time.second =
                    initial_assembly_upload_start_and_finish_time.second + violation_amount;
            initial_conv_block->assembly_upload_windows[initial_partition_id]->setStartFinTime(
                    initial_assembly_upload_start_and_finish_time);
        } else {
            /* Otherwise no significant violation has occurred and we can leave things as they are*/
            web::json::value log;
            log["dnn_id"] = web::json::value(violated_dnn->getDnnId());
            queueManager->logManager->add_log(enums::LogTypeEnum::DAG_DISRUPTION_SUCCESS, log);
            services::WorkQueueManager::decrementThreadCounter();
            net_lock.unlock();
            offload_lock.unlock();
            return;
        }

        if (initial_assembly_upload_start_and_finish_time.second > initial_conv_block->getAssemblyStartTime()) {
            auto violation_amount =
                    initial_assembly_upload_start_and_finish_time.second - initial_conv_block->getAssemblyStartTime();
            initial_conv_block->setAssemblyStartTime(initial_conv_block->getAssemblyStartTime() + violation_amount);
            initial_conv_block->setStateUpdateFinTime(initial_conv_block->getStateUpdateFinTime());
        } else {
            /* Otherwise no significant violation has occurred and we can leave things as they are*/
            web::json::value log;
            log["dnn_id"] = web::json::value(violated_dnn->getDnnId());
            queueManager->logManager->add_log(enums::LogTypeEnum::DAG_DISRUPTION_SUCCESS, log);
            net_lock.unlock();
            offload_lock.unlock();
            services::WorkQueueManager::decrementThreadCounter();
            return;
        }

        auto initial_state_update = initial_conv_block->state_update;
        auto initial_state_update_start_fin = initial_state_update->getStartFinTime();
        /* Need to check that the assembly finish time doesn't violate the state_update time */
        if (initial_conv_block->getAssemblyFinTime() > initial_state_update_start_fin.first) {
            auto violation_amount = initial_conv_block->getAssemblyFinTime() - initial_state_update_start_fin.first +
                                    std::chrono::milliseconds{1};
            initial_state_update_start_fin.first = initial_state_update_start_fin.first + violation_amount;
            initial_state_update_start_fin.second = initial_state_update_start_fin.second + violation_amount;
            initial_state_update->setStartFinTime(initial_state_update_start_fin);
            initial_conv_block->setStateUpdateFinTime(initial_state_update_start_fin.second);
        } else {
            /* Otherwise no significant violation has occurred and we can leave things as they are*/
            web::json::value log;
            log["dnn_id"] = web::json::value(violated_dnn->getDnnId());
            queueManager->logManager->add_log(enums::LogTypeEnum::DAG_DISRUPTION_SUCCESS, log);
            net_lock.unlock();
            offload_lock.unlock();
            services::WorkQueueManager::decrementThreadCounter();
        }

        /* If this is set to true during iteration,
         * exit the violation trace and issue a prune request */
        bool prune_dnn = false;
        std::string conv_block_to_update_to;

        for (int convidx_int = std::stoi(initial_condidx) + 1;
             convidx_int <= violated_dnn->tasks.size(); convidx_int++) {
            auto conv_block = violated_dnn->tasks[std::to_string(convidx_int)];
            auto previous_conv_block = violated_dnn->tasks[std::to_string(convidx_int - 1)];
            /* If the state_update is modified at the end then we must continue tracing */
            bool continue_trace = false;

            for (auto [partition_id, partition]: conv_block->partitioned_tasks) {
                int unique_task_id = partition->getUniqueTaskId();

                auto input_upload = partition->getInputData();
                auto input_upload_start_finish_time = input_upload->getStartFinTime();
                if (previous_conv_block->getStateUpdateFinTime() > input_upload_start_finish_time.first) {
                    auto violation =
                            previous_conv_block->getStateUpdateFinTime() - input_upload_start_finish_time.first +
                            std::chrono::milliseconds{1};
                    input_upload_start_finish_time.first += violation;
                    input_upload_start_finish_time.second += violation;
                } else
                    continue;
                std::chrono::time_point<std::chrono::system_clock> old_finish_time;
                if (input_upload_start_finish_time.second > partition->getEstimatedStart()) {
                    old_finish_time = partition->getEstimatedFinish();
                    auto violation = input_upload_start_finish_time.second - partition->getEstimatedStart() +
                                     std::chrono::milliseconds{1};
                    partition->setEstimatedStart(partition->getEstimatedStart() + violation);
                    partition->setEstimatedFinish(partition->getEstimatedFinish() + violation);
                } else
                    continue;

                task_counter = 0;
                for (const auto &task: queueManager->network->getDevices()[partition->getAllocatedHost()]->TASKS) {
                    if (task->getUniqueTaskId() != unique_task_id &&
                        task->getEstimatedStart() >= old_finish_time &&
                        task->getEstimatedFinish() <= task->getEstimatedFinish())
                        task_counter++;
                }

                if (task_counter == 4) {
                    prune_dnn = true;
                    break;
                }

                auto upload_start_and_finish_time = conv_block->assembly_upload_windows[partition_id]->getStartFinTime();
                /* Need to check to see the violated tasks new finish time is greater than its upload start,
                 * if so we need to now update the upload windows, if not then we can end the violation trace here */
                if (partition->getEstimatedFinish() >= upload_start_and_finish_time.first) {
                    auto violation_amount =
                            partition->getEstimatedFinish() - upload_start_and_finish_time.first +
                            std::chrono::milliseconds{1};
                    upload_start_and_finish_time.first = upload_start_and_finish_time.first + violation_amount;
                    upload_start_and_finish_time.second = upload_start_and_finish_time.second + violation_amount;
                    conv_block->assembly_upload_windows[partition_id]->setStartFinTime(
                            upload_start_and_finish_time);
                }
                    /* TODO Otherwise no significant violation in this task has occurred and we should continue */
                else
                    continue;

                if (upload_start_and_finish_time.second > conv_block->getAssemblyStartTime()) {
                    auto violation_amount = upload_start_and_finish_time.second - conv_block->getAssemblyStartTime();
                    conv_block->setAssemblyStartTime(conv_block->getAssemblyStartTime() + violation_amount);
                    conv_block->setStateUpdateFinTime(conv_block->getStateUpdateFinTime());

                }
                    /* TODO Otherwise no significant violation in this task has occurred and we should continue */
                else
                    continue;

                auto state_update = conv_block->state_update;
                auto state_update_start_fin = state_update->getStartFinTime();
                /* Need to check that the assembly finish time doesn't violate the state_update time */
                if (conv_block->getAssemblyFinTime() > state_update_start_fin.first) {
                    auto violation_amount = conv_block->getAssemblyFinTime() - state_update_start_fin.first +
                                            std::chrono::milliseconds{1};
                    state_update_start_fin.first = state_update_start_fin.first + violation_amount;
                    state_update_start_fin.second = state_update_start_fin.second + violation_amount;
                    state_update->setStartFinTime(state_update_start_fin);
                    conv_block->setStateUpdateFinTime(state_update_start_fin.second);
                    continue_trace = true;
                }
            }

            if (prune_dnn || !continue_trace) {
                conv_block_to_update_to = (convidx_int == violated_dnn->tasks.size()) ? std::to_string(convidx_int)
                                                                                      : std::to_string(convidx_int + 1);
                break;
            }
        }

        net_lock.unlock();
        offload_lock.unlock();

        if (violated_dnn->tasks[std::to_string(violated_dnn->tasks.size())]->getStateUpdateFinTime() >
            violated_dnn->getDeadline())
            prune_dnn = true;

        if (prune_dnn) {
            auto pruneItem = std::make_shared<model::PruneItem>(enums::request_type::prune_dnn,
                                                                violated_dnn->getDnnId(),
                                                                conv_block_to_update_to);

            web::json::value log;
            log["dnn_id"] = web::json::value(violated_dnn->getDnnId());
            queueManager->logManager->add_log(enums::LogTypeEnum::DAG_DISRUPTION_FAIL, log);
            queueManager->add_task(std::static_pointer_cast<model::WorkItem>(pruneItem));
        } else {
            uint64_t old_version = violated_dnn->getVersion();
            violated_dnn->setVersion(std::chrono::system_clock::now().time_since_epoch().count() * 1000);
            auto outboundUpdate = std::make_shared<model::OutboundUpdate>(enums::network_comms_types::task_update,
                                                                          std::chrono::system_clock::now(),
                                                                          violated_dnn, conv_block_to_update_to, old_version);

            web::json::value log;
            log["dnn_id"] = web::json::value(violated_dnn->getDnnId());
            queueManager->logManager->add_log(enums::LogTypeEnum::DAG_DISRUPTION_SUCCESS, log);

            queueManager->networkQueueManager->addTask(
                    std::static_pointer_cast<model::BaseNetworkCommsModel>(outboundUpdate));
        }

        services::WorkQueueManager::decrementThreadCounter();
    }

    void halt_call(std::shared_ptr<WorkQueueManager> queueManager) {
        std::vector<std::string> hostList;
        for (auto [host_id, host]: queueManager->network->getDevices()) {
            hostList.push_back(host_id);
        }

        std::unique_lock<std::mutex> offload_lock(queueManager->offloaded_lock, std::defer_lock);
        offload_lock.lock();

        std::map<std::string, uint64_t> result_json;
        for (auto [dnn_id, dnn]: queueManager->off_high)
            result_json[dnn_id] = dnn->getVersion();

        std::shared_ptr<model::HaltNetworkCommsModel> baseNetworkCommsModel = std::make_shared<model::HaltNetworkCommsModel>(
                enums::network_comms_types::halt_req,
                std::chrono::system_clock::now(), result_json);

        queueManager->networkQueueManager->addTask(baseNetworkCommsModel);

        std::vector<std::shared_ptr<model::HighCompResult>> highProcessingList;


        std::unique_lock<std::mutex> network_lock(queueManager->network_lock, std::defer_lock);

        //Need to record incomplete tasks for reallocation (device allocated to, task_id)
        std::vector<std::pair<std::string, int>> devices_and_tasks_to_prune;
        std::vector<std::shared_ptr<model::LinkAct>> links_to_prune;

        for (auto [dnn_id, dnn]: queueManager->off_high) {
            std::string last_complete_block = "";

            for (auto [convidx, block]: dnn->tasks) {

                if (!block->isCompleted()) {
                    for (auto [partition_id, task]: block->partitioned_tasks) {
                        //Need to clear allocation details
                        task->setEstimatedFinish(std::chrono::time_point<std::chrono::system_clock>{
                                std::chrono::milliseconds(-1)});
                        task->setEstimatedStart(std::chrono::time_point<std::chrono::system_clock>{
                                std::chrono::milliseconds(-1)});
                        std::shared_ptr<model::LinkAct> new_link_act;

                        links_to_prune.push_back(task->getInputData());

                        task->setInputData(new_link_act);

                        std::string hostname = task->getAllocatedHost();

                        task->setAllocatedHost("");
                        task->setTaskOutputSizeBytes(-1);

                        devices_and_tasks_to_prune.emplace_back(hostname, task->getUniqueTaskId());
                    }
                    block->partitioned_tasks.clear();
                    for (auto [id, link_act]: block->assembly_upload_windows) {
                        links_to_prune.push_back(link_act);
                        link_act.reset();
                    }
                    block->assembly_upload_windows.clear();
                    links_to_prune.push_back(block->state_update);
                    block->state_update.reset();
                } else
                    last_complete_block = convidx;
            }
            dnn->setLastCompleteConvIdx(std::stoi(last_complete_block));
            links_to_prune.push_back(dnn->getUploadData());
            highProcessingList.push_back(dnn);
        }

        network_lock.lock();

        //NEED TO REMOVE TASKS FROM TOTAL LIST AND
        for (auto [device_name, task_id]: devices_and_tasks_to_prune) {
            auto device = queueManager->network->getDevices()[device_name];
            auto it = device->TASKS.begin();
            int index = 0;

            while (it != device->TASKS.end()) {
                if (task_id == device->getTasks()[index]->getUniqueTaskId()) {
                    device->TASKS.erase(it);
                    break;
                } else {
                    it++;
                    index++;
                }
            }
        }

        for (const auto &link: links_to_prune) {
            int link_act_id = link->getLinkActivityId();

            auto it = queueManager->network->network_link.begin();

            while (it != queueManager->network->network_link.end()) {
                if (link_act_id == link->getLinkActivityId()) {
                    queueManager->network->network_link.erase(it);
                    break;
                } else
                    it++;
            }
        }

        network_lock.unlock();
        offload_lock.unlock();

        for (auto dnn: highProcessingList) {
            std::string last_complete_conv = std::to_string(dnn->getLastCompleteConvIdx());
            std::string data_host = dnn->tasks[last_complete_conv]->getAssemblyHost();
            auto host_list = std::make_shared<std::vector<std::string>>(std::initializer_list<std::string>{data_host});
            auto highComplexityTask = std::make_shared<model::HighProcessingItem>(
                    host_list, enums::request_type::high_complexity, dnn->getDeadline(),
                    dnn->getDnnId());

            highComplexityTask->setReallocation(true);
            queueManager->add_task(highComplexityTask);
        }

        services::WorkQueueManager::decrementThreadCounter();
    }

    void WorkQueueManager::add_task(std::shared_ptr<model::WorkItem> item) {
        std::unique_lock<std::mutex> lk(WorkQueueManager::work_queue_lock, std::defer_lock);

        web::json::value log;
        WorkQueueManager::logManager->add_log(enums::LogTypeEnum::ADD_WORK_TASK, log);
        lk.lock();
        /* No more than one halt request can be present at any one time, if we find a DAG disruption request in the list
         * when we add a halt to the queue then we need to remove it, as a halt request will lead to full reallocation
         * making any DAG disruption request redundant */
        if (item->getRequestType() == enums::request_type::halt_req) {
            auto it = WorkQueueManager::work_queue.begin();
            int index = 0;
            while (it != WorkQueueManager::work_queue.end()) {
                if (WorkQueueManager::work_queue[index]->getRequestType() == enums::request_type::dag_disruption) {
                    WorkQueueManager::work_queue.erase(it);
                } else if (WorkQueueManager::work_queue[index]->getRequestType() == enums::request_type::halt_req)
                    return;
                else {
                    ++index;
                    ++it;
                }
            }
        }
            /* If we have a DAG disruption request they cannot coexist with a halt request */
        else if (item->getRequestType() == enums::request_type::dag_disruption) {
            int list_size = static_cast<int>(WorkQueueManager::work_queue.size());
            for (int i = 0; i < list_size; i++) {
                if (WorkQueueManager::work_queue[i]->getRequestType() == enums::request_type::halt_req)
                    return;
            }
        }

        WorkQueueManager::work_queue.push_back(item);

        std::sort(WorkQueueManager::work_queue.begin(), WorkQueueManager::work_queue.end(),
                  [](std::shared_ptr<model::WorkItem> a, std::shared_ptr<model::WorkItem> b) {

                      if (a->getRequestType() == b->getRequestType()) {
                          if (a->getRequestType() == enums::request_type::dag_disruption) {
                              auto a_cast = std::static_pointer_cast<model::DAGDisruption>(a);
                              auto b_cast = std::static_pointer_cast<model::DAGDisruption>(b);

                              return a_cast->getFinishTime() < b_cast->getFinishTime();
                          } else if (a->getRequestType() == enums::request_type::low_complexity) {
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

    [[noreturn]] void WorkQueueManager::main_loop(std::shared_ptr<WorkQueueManager> queueManager) {
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
                        thread_pool.emplace_back(low_comp_allocation_call, queueManager->current_task.front(), queueManager);
                        break;
                    case enums::request_type::high_complexity:
                        thread_pool.emplace_back(high_comp_allocation_call, queueManager->current_task.front(), queueManager);
                        break;
                    case enums::request_type::dag_disruption:
                        thread_pool.emplace_back(dag_disruption_call, queueManager->current_task.front(), queueManager);
                        break;
                    case enums::request_type::prune_dnn:
                        thread_pool.emplace_back(prune_dnn_call, queueManager->current_task.front(), queueManager);
                        break;
                    case enums::request_type::halt_req:
                        thread_pool.emplace_back(halt_call, queueManager);
                        break;
                    case enums::request_type::state_update:
                        thread_pool.emplace_back(state_update_call, queueManager->current_task.front(), queueManager);
                        break;
                }

                /* We wait for the current work items to terminate */
                while (queueManager->thread_counter > 0) {
                    lk.lock();
                    if (queueManager->work_queue.front()->getRequestType() == enums::request_type::low_complexity) {
                        /* If more low complexity tasks are added to the queue then we append them to the current
                         * work list */
                        while (queueManager->work_queue.front()->getRequestType() ==
                               enums::request_type::low_complexity) {
                            queueManager->thread_counter++;
                            thread_pool.emplace_back(low_comp_allocation_call, queueManager->work_queue.front(),
                                                     queueManager);
                            queueManager->current_task.push_back(queueManager->work_queue.front());
                            queueManager->work_queue.erase(queueManager->work_queue.begin());
                        }
                    }
                    lk.unlock();
                }
                for (auto &thread: thread_pool)
                    thread.detach();
                thread_pool.clear();
            }
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

    WorkQueueManager::WorkQueueManager(std::shared_ptr<LogManager> ptr, std::shared_ptr<NetworkQueueManager> sharedPtr)
            : logManager(std::move(ptr)) {
        WorkQueueManager::lookup_table = utils::parseFTP_Lookup();
        WorkQueueManager::state_size = utils::calculateSizeOfInputData(lookup_table);
        WorkQueueManager::network = std::make_shared<model::Network>();
        WorkQueueManager::networkQueueManager = sharedPtr;

    }

    double WorkQueueManager::getBytesPerMillisecond() {
        /* Fetching the bandwidth and latency */
        double bw_bytes_per_second = (WorkQueueManager::getAverageBitsPerSecond() / 8);
        double latency_bytes = WorkQueueManager::getJitter() / 8;
        bw_bytes_per_second += latency_bytes;
        double bw_bytes_per_ms = bw_bytes_per_second / 1000;
        return bw_bytes_per_ms;
    }

} // services