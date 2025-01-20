//
// Created by jamiec on 9/23/22.
//

#include <thread>
#include <random>
#include <utility>
#include <vector>
#include "WorkQueueManager.h"
#include "../../model/data_models/NetworkCommsModels/BandwidthTestCommsModel/BandwidthTestCommsModel.h"
#include "../../model/data_models/WorkItems/ProcessingItem/HighProcessingItem/HighProcessingItem.h"
#include "../../model/data_models/WorkItems/StateUpdate/StateUpdate.h"
#include "../../Constants/ModeMacros.h"
#include "../../Constants/CLIENT_DETAILS.h"
#include "../../Constants/AllocationMacros.h"
#include "../../model/data_models/WorkItems/ProcessingItem/LowProcessingItem/LowProcessingItem.h"
#include "../../model/data_models/NetworkCommsModels/LowComplexityAllocation/LowComplexityAllocationComms.h"
#include "../../model/data_models/WorkItems/PruneItem/PruneItem.h"
#include "../../model/data_models/NetworkCommsModels/HaltNetworkCommsModel/HaltNetworkCommsModel.h"
#include "../../model/data_models/WorkItems/HaltWorkItem/HaltWorkItem.h"
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

            auto variance = (1 - EMA_ALPHA) * (old_jitter + diff * increment);

            queueManager->setAverageBitsPerSecond(mean);
            queueManager->setJitter(variance);
        }

        web::json::value log;
        log["new_bps"] = web::json::value::number(queueManager->getAverageBitsPerSecond());
        log["new_jitter"] = web::json::value::number(queueManager->getJitter());

        auto bps = queueManager->getBytesPerMillisecond();
        if (bps < 0) {
            std::cerr << "ERROR: BANDWIDTH NEGATIVE VALUE" << std::endl;
            queueManager->getBytesPerMillisecond();
        }

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

        auto host = (*stateUpdate->getHostList())[0];

        std::shared_ptr<model::BaseCompResult> dnn;

        auto dnn_id = stateUpdate->getDnnId();
        for (auto task: queueManager->network->devices[host]->DNNS) {
            if (task->getDnnId() == dnn_id) {
                dnn = task;
                break;
            }
        }

        // For the condition where a halt was sent out
        // but a task was already being sent back to scheduler
        if (dnn == nullptr) {
            off_lock.unlock();
            queueManager->decrementThreadCounter();
            return;
        }

        std::unique_lock<std::mutex> net_lock(queueManager->network_lock, std::defer_lock);

        net_lock.lock();

        std::string allocated_device = dnn->getAllocatedHost();

        queueManager->network->devices[allocated_device]->DNNS.erase(
                std::remove_if(queueManager->network->devices[allocated_device]->DNNS.begin(),
                               queueManager->network->devices[allocated_device]->DNNS.end(),
                               [dnn_id](std::shared_ptr<model::BaseCompResult> br) {
                                   return dnn_id == br->getDnnId();
                               }), queueManager->network->devices[allocated_device]->DNNS.end());


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

    /*Function receives a  nlow complexity DNN allocation request*/
    void low_comp_allocation_call(std::shared_ptr<model::WorkItem> item, WorkQueueManager *queueManager) {
        auto proc_item = std::static_pointer_cast<model::LowProcessingItem>(item);

        std::unique_lock<std::mutex> net_lock(queueManager->network_lock, std::defer_lock);

        std::map<std::string, std::shared_ptr<model::ComputationDevice>> &devices = queueManager->network->devices;

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

        net_lock.lock();
        auto device = queueManager->network->devices[host];
        net_lock.unlock();

        auto res = device->resource_avail_windows[1]->containmentQuery(
                currentTime, estimated_fin);

        auto index = res.first;
        auto window = res.second;
        /* If we cannot allocate a device for even one task we instead create a new allocation request and a halt request */
        if (index == TASK_NOT_FOUND) {

#if DEADLINE_PREMPT
            std::shared_ptr<model::WorkItem> workItem = std::make_shared<model::HaltWorkItem>(
                    enums::request_type::halt_req, host, currentTime,
                    estimated_fin);

            queueManager->add_task(workItem);

            proc_item->setReallocation(true);

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


#if DEADLINE_PREMPT
            queueManager->logManager->add_log(enums::LogTypeEnum::LOW_COMP_PREEMPTION, log);
            queueManager->decrementThreadCounter();
            return;
#else
            queueManager->logManager->add_log(enums::LogTypeEnum::LOW_COMP_ALLOCATION_FAIL, log);
            queueManager->decrementThreadCounter();
            return;
#endif
        }
        /* For each allocated low comp task we create a Result object
         * the key is the dnn_id*/

        auto deadline = proc_item->getDeadline();
        auto bR = std::make_shared<model::LowCompResult>(dnn_id, host, 1, deadline, currentTime,
                                                         estimated_fin,
                                                         enums::dnn_type::low_comp
        );

        std::unique_lock<std::mutex> offload_lock(queueManager->offloaded_lock, std::defer_lock);

        /* Add communication times to the network link */
        bR->setAllocatedHost(host);

        std::shared_ptr<model::LowComplexityAllocationComms> baseNetworkCommsModel =
                std::make_shared<model::LowComplexityAllocationComms>(
                        enums::network_comms_types::low_complexity_allocation,
                        currentTime - std::chrono::milliseconds{data_size_bytes / bw_bytes},
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
        queueManager->network->devices[host]->resAvailRemoveAndSplit(tw, LOW_COMPLEXITY_CORE_COUNT, 0);
        queueManager->decrementThreadCounter();
    }

    int selectResourceConfig(std::chrono::time_point<std::chrono::system_clock> start,
                             std::chrono::time_point<std::chrono::system_clock> deadline) {
        int resourceConfig = 2;
        if ((deadline - start) < std::chrono::milliseconds{FTP_LOW_TIME}) {
            resourceConfig = 4;
            if ((deadline - start) < std::chrono::milliseconds{FTP_HIGH_TIME}) {
                resourceConfig = TASK_NOT_FOUND;
            }
        }
        return resourceConfig;
    }

    std::vector<int> gatherCommSlots(int task_count, std::vector<std::shared_ptr<model::Bucket>> netlink,
                                     std::chrono::time_point<std::chrono::system_clock> currentTime,
                                     std::chrono::time_point<std::chrono::system_clock> last_time_of_reasoning,
                                     double bytes_per_millisecond
    ) {
        /* Gather Comm Windows */
        std::vector<int> task_comm_windows = {};

        auto ct_uint = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
                currentTime.time_since_epoch()).count());
        auto last_reason_uint = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
                last_time_of_reasoning.time_since_epoch()).count());
        auto base_comm_size = static_cast<uint64_t>(TASK_FORWARD_SIZE / bytes_per_millisecond);

        for (int i = 0; i < task_count; i++) {
            uint64_t index = services::obtain_index(ct_uint, last_reason_uint, base_comm_size, BASE_BUCKET_COUNT);
            while (netlink[index]->bucketContents.size() == netlink[index]->capacity)
                index += 1;

            currentTime = netlink[index]->timeWindow->stop;
            ct_uint = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
                    currentTime.time_since_epoch()).count());
            task_comm_windows.push_back(static_cast<int>(index));
        }
        return task_comm_windows;
    }

    void findResourceWindow(std::chrono::time_point<std::chrono::system_clock> start_time,
                            std::chrono::time_point<std::chrono::system_clock> deadline,
                            std::shared_ptr<model::ComputationDevice> device, int resourceConfig, int task_count,
                            std::string host,
                            std::map<std::string, std::vector<std::pair<int, std::shared_ptr<model::ResourceWindow>>>> &results,
                            std::mutex &mtx) {
        auto res = device->resource_avail_windows[resourceConfig]->containmentQueryMulti(start_time, deadline,
                                                                                         task_count);

        std::lock_guard<std::mutex> guard(mtx);
        results[host] = res;
    }

    std::pair<std::vector<std::pair<int, std::shared_ptr<model::ResourceWindow>>>, std::vector<std::pair<int, std::shared_ptr<model::ResourceWindow>>>>
    selectResults(
            std::map<std::string, std::vector<std::pair<int, std::shared_ptr<model::ResourceWindow>>>> placement_results,
            std::string sourceHost, int task_count) {
        std::vector<std::pair<int, std::shared_ptr<model::ResourceWindow>>> source_selected_results;
        std::vector<std::pair<int, std::shared_ptr<model::ResourceWindow>>> offloaded_selected_results;

        for (auto result: placement_results[sourceHost]) {
            if (source_selected_results.size() != task_count)
                source_selected_results.push_back(result);
            else
                break;
        }

        placement_results.erase(sourceHost);

        for (auto [host, placement_result]: placement_results) {
            for (auto result: placement_result) {
                if (source_selected_results.size() + offloaded_selected_results.size() != task_count)
                    offloaded_selected_results.push_back(result);
                else break;
            }
            if (source_selected_results.size() + offloaded_selected_results.size() == task_count)
                break;
        }

        return std::make_pair(source_selected_results, offloaded_selected_results);
    }


    std::tuple<std::vector<int>, std::vector<std::shared_ptr<model::HighCompResult>>, std::vector<std::shared_ptr<model::HighComplexityAllocationComms>>>
    generateHighCompResults(
            std::vector<std::string> dnnIds,
            std::vector<std::pair<int, std::shared_ptr<model::ResourceWindow>>> source_selected_results,
            std::vector<std::pair<int, std::shared_ptr<model::ResourceWindow>>> offloaded_selected_results,
            std::vector<std::shared_ptr<model::Bucket>> network_link,
            std::vector<int> task_comm_windows,
            std::string sourceHost,
            int resourceConfig,
            std::chrono::time_point<std::chrono::system_clock> currentTime,
            uint64_t processing_time,
            int n,
            int m,
            std::chrono::time_point<std::chrono::system_clock> deadline,
            bool isReallocation
    ) {
        int source_select_counter = 0;
        int offloaded_select_counter = 0;

        std::vector<int> selected_link_results;
        std::vector<std::shared_ptr<model::HighCompResult>> high_comp_results;
        std::vector<std::shared_ptr<model::HighComplexityAllocationComms>> high_complexity_comms;
        std::chrono::time_point<std::chrono::system_clock> ct = currentTime;

        for (const auto &dnn_id: dnnIds) {
            std::shared_ptr<model::Bucket> selectedBucket = nullptr;
            std::shared_ptr<model::ResourceWindow> resource_window;
            if (source_selected_results.size() == source_select_counter &&
                offloaded_selected_results.size() == offloaded_select_counter)
                break;
            if (source_selected_results.size() != source_select_counter) {
                auto [res_index, res_window] = source_selected_results[source_select_counter];
                resource_window = res_window;
                source_select_counter++;
            } else {
                auto [res_index, res_window] = offloaded_selected_results[offloaded_select_counter];
                resource_window = res_window;
                const auto &bucket = network_link[task_comm_windows[offloaded_select_counter]];
                selectedBucket = bucket;
                ct = bucket->timeWindow->stop;
                selected_link_results.push_back(task_comm_windows[offloaded_select_counter]);
                offloaded_select_counter++;
            }

            auto high_comp_res = std::make_shared<model::HighCompResult>(dnn_id, resource_window->deviceId, sourceHost,
                                                                         resourceConfig, deadline,
                                                                         ct,
                                                                         ct +
                                                                         std::chrono::milliseconds{processing_time},
                                                                         enums::dnn_type::high_comp, n, m,
                                                                         selectedBucket);

            if (isReallocation)
                high_comp_res->setVersion(std::chrono::system_clock::now().time_since_epoch().count() * 1000);

            high_comp_results.push_back(high_comp_res);

            high_complexity_comms.push_back(std::make_shared<model::HighComplexityAllocationComms>(
                    enums::network_comms_types::high_complexity_task_mapping,
                    std::chrono::system_clock::now(),
                    high_comp_res,
                    sourceHost));
        }
        return std::make_tuple(selected_link_results, high_comp_results, high_complexity_comms);
    }

    std::tuple<int, int, uint64_t> selectResourceUsage(int resourceConfig) {
        int n = FTP_LOW_N;
        int m = FTP_LOW_M;
        uint64_t processing_time = FTP_LOW_TIME;
        if (resourceConfig == FTP_HIGH_CORE) {
            n = FTP_HIGH_N;
            m = FTP_HIGH_M;
            processing_time = FTP_HIGH_TIME;
        }

        return std::make_tuple(n, m, processing_time);
    }


    void high_comp_allocation_call(std::shared_ptr<model::WorkItem> item, WorkQueueManager *queueManager) {
        auto processingItem = std::static_pointer_cast<model::HighProcessingItem>(item);
        bool isReallocation = processingItem->isReallocation();

        std::string sourceHost = (*item->getHostList())[0];

        std::map<std::string, std::shared_ptr<model::HighCompResult>> baseResult;

        auto deadline = processingItem->getDeadline();

        std::chrono::time_point<std::chrono::system_clock> currentTime =
                std::chrono::system_clock::now() + std::chrono::milliseconds{HIGH_COMP_START_TIME_OFFSET_MS};

        /* Selecting which config to use based on processing viability */
        int resourceConfig = selectResourceConfig(currentTime, processingItem->getDeadline());

        if (resourceConfig == TASK_NOT_FOUND) {
            web::json::value log;
            log["dnn_id"] = web::json::value::string(processingItem->getDnnId());
            queueManager->logManager->add_log((isReallocation) ? enums::LogTypeEnum::HIGH_COMP_REALLOCATION_FAIL
                                                               : enums::LogTypeEnum::HIGH_COMP_ALLOCATION_FAIL,
                                              log);

            queueManager->decrementThreadCounter();
            return;

        }

        /* Gathering the FTP Config based on core usage */
        auto [n, m, processing_time] = selectResourceUsage(resourceConfig);

        /* Gather Comm Windows */
        int task_count = static_cast<int>(processingItem->getDnnIds().size());
        std::vector<int> task_comm_windows = gatherCommSlots(task_count, queueManager->network->network_link,
                                                             currentTime, queueManager->network->last_time_of_reasoning,
                                                             queueManager->getBytesPerMillisecond());

        /* Find resource windows that overlap with our desired window */
        std::map<std::string, std::vector<std::pair<int, std::shared_ptr<model::ResourceWindow>>>> placement_results;

        std::mutex mtx;
        std::vector<std::thread> thread_pool;
        for (const auto &[hostname, device]: queueManager->network->devices) {
            thread_pool.emplace_back(findResourceWindow, currentTime, deadline, device, resourceConfig, task_count,
                                     hostname,
                                     std::ref(placement_results), std::ref(mtx));
        }

        bool all_joined = false;
        while (!all_joined) {
            all_joined = true; // Assume all threads are joined initially.
            for (auto &thread_item: thread_pool) {
                if (thread_item.joinable()) {
                    thread_item.join();
                    all_joined = false; // A joinable thread was found and joined, so we keep looping.
                }
            }
        }

        /* Filtering the resource windows, prioritising local device */
        auto [source_selected_results, offloaded_selected_results] = selectResults(placement_results, sourceHost,
                                                                                   task_count);


        /* If there are no valid resource windows, exit */
        if (source_selected_results.empty() && offloaded_selected_results.empty()) {
            web::json::value log;
            log["dnn_id"] = web::json::value::string(processingItem->getDnnId());
            queueManager->logManager->add_log((isReallocation) ? enums::LogTypeEnum::HIGH_COMP_REALLOCATION_FAIL
                                                               : enums::LogTypeEnum::HIGH_COMP_ALLOCATION_FAIL,
                                              log);

            queueManager->decrementThreadCounter();
            return;
        }

        /* Generating state data structures */
        auto [selected_link_results, hcr, high_comp_comms] = generateHighCompResults(
                processingItem->getDnnIds(),
                source_selected_results,
                offloaded_selected_results,
                queueManager->network->network_link,
                task_comm_windows, sourceHost,
                resourceConfig, currentTime,
                processing_time, n, m, deadline, isReallocation);

        /* Sending allocated tasks outbound */
        for (auto commItem: high_comp_comms) {
            queueManager->networkQueueManager->addTask(
                    std::static_pointer_cast<model::BaseNetworkCommsModel>(commItem));

        }

        auto high_comp_results = hcr;

        /* LOGGING RESULTS and Adding RESULTS to network state */
        for (int i = 0; i < processingItem->getDnnIds().size(); i++) {
            if (i < high_comp_results.size()) {
                /* Logging */
                web::json::value log;
                log["dnn"] = web::json::value(high_comp_results[i]->convertToJson());
                queueManager->logManager->
                        add_log((isReallocation)
                                ? enums::LogTypeEnum::HIGH_COMP_REALLOCATION_SUCCESS
                                : enums::LogTypeEnum::HIGH_COMP_ALLOCATION_SUCCESS, log);

                /* Adding task to device */
                queueManager->network->devices[high_comp_results[i]->getAllocatedHost()]->DNNS.push_back(
                        high_comp_results[i]);

                /* Marking bucket as occupied */
                if (high_comp_results[i]->getSrcHost() != high_comp_results[i]->getAllocatedHost())
                    high_comp_results[i]->getTaskAllocation()->bucketContents.push_back(
                            high_comp_results[i]->getDnnId());

                /* Modifying resource windows */
                queueManager->network->devices[high_comp_results[i]->getAllocatedHost()]->resAvailRemoveAndSplit(
                        high_comp_results[i]->estimated_start_fin, resourceConfig, i);
            } else {
                /* Logging failure */
                web::json::value log;
                log["dnn_id"] = web::json::value::string(processingItem->getDnnIds()[i]);
                queueManager->logManager->add_log((isReallocation) ? enums::LogTypeEnum::HIGH_COMP_REALLOCATION_FAIL
                                                                   : enums::LogTypeEnum::HIGH_COMP_ALLOCATION_FAIL,
                                                  log);
            }
        }


        queueManager->decrementThreadCounter();
    }

    void halt_call(std::shared_ptr<model::WorkItem> workItem, WorkQueueManager *queueManager) {
        auto haltWorkItem = std::static_pointer_cast<model::HaltWorkItem>(workItem);
        std::vector<std::string> hostList = queueManager->networkQueueManager->getHosts();

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
            if ((max(startTime, tasks[i]->estimated_start_fin->start) -
                 min(finTime, tasks[i]->estimated_start_fin->stop)).count() <= 0 &&
                tasks[i]->getDnnType() == enums::dnn_type::high_comp)
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

        if (dnnToPrune == nullptr) {
            auto regenDataStr = std::make_shared<model::WorkItem>(
                    std::make_shared<std::vector<std::string>>(std::vector<std::string>{sourceDeviceId}),
                    enums::request_type::regenerate_structure);
            queueManager->add_task(regenDataStr);
            queueManager->decrementThreadCounter();
            return;
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
        dnnToPrune->estimated_start_fin->start = std::chrono::time_point<std::chrono::system_clock>(
                std::chrono::milliseconds{0});
        dnnToPrune->estimated_start_fin->stop = std::chrono::time_point<std::chrono::system_clock>(
                std::chrono::milliseconds{0});
        dnnToPrune->setVersion(std::chrono::system_clock::now().time_since_epoch().count() * 1000);
        dnnToPrune->setN(0);
        dnnToPrune->setM(0);

        dnnToPrune->setAllocatedHost("");

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

        auto regenDataStr = std::make_shared<model::WorkItem>(
                std::make_shared<std::vector<std::string>>(std::vector<std::string>{sourceDeviceId}),
                enums::request_type::regenerate_structure);
        queueManager->add_task(regenDataStr);
        queueManager->add_task(highProcessingItem);
        queueManager->decrementThreadCounter();
    }

    void regenerate_res_data_structure(std::shared_ptr<model::WorkItem> workItem, WorkQueueManager *queueManager) {
        auto host = (*workItem->getHostList())[0];
        auto device = queueManager->network->devices[host];

        if(device->DNNS.empty()){
            device->generateDefaultResourceConfig(device->getCores(), device->getHostName());
        }
        else {
            std::chrono::time_point<std::chrono::system_clock> ct = std::chrono::system_clock::now();

            std::vector<int> res_usage_list;

            for (auto [res_usage, res_config]: device->resource_avail_windows) {
                res_usage_list.push_back(res_usage);
            }

            for (auto res_usage: res_usage_list) {
                device->resource_avail_windows[res_usage]->resource_windows = services::convertOverlapToResourceAvailability(
                        device->DNNS, device->resource_avail_windows[res_usage]->task_res_usage, ct, host,
                        device->resource_avail_windows[res_usage]->min_processing_time);
            }
        }
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
            volatile bool wait = false;
            while (!queueManager->start)
                wait;

            update_network_disc(nullptr, queueManager);

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
                        case enums::request_type::regenerate_structure:
                            thread_pool.emplace_back(regenerate_res_data_structure, queueManager->current_task.front(),
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
            : logManager(std::move(ptr)) {
        WorkQueueManager::network = std::make_shared<model::Network>();
        WorkQueueManager::networkQueueManager = sharedPtr;

    }

    double WorkQueueManager::getBytesPerMillisecond() {
        /* Fetching the bandwidth and latency */
        double bw_bytes_per_second = (WorkQueueManager::getAverageBitsPerSecond() / 8);
        double latency_bytes = WorkQueueManager::getJitter() / 8;
        //TODO INVESTIGATE WHY THIS IS RESULTING IN A JITTER LARGER THAN THE DAMN MEAN
//        bw_bytes_per_second -= latency_bytes;
        double bw_bytes_per_ms = bw_bytes_per_second / 1000;
        return bw_bytes_per_ms;
    }

} // services