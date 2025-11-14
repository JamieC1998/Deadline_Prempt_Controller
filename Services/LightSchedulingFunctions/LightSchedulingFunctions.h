//
// Created by Jamie Cotter on 28/07/2025.
//

#ifndef CONTROLLER_LIGHTSCHEDULINGFUNCTIONS_H
#define CONTROLLER_LIGHTSCHEDULINGFUNCTIONS_H

#include <memory>
#include <vector>
#include <chrono>
#include <map>
#include "../../model/data_models/Bucket/Bucket.h"
#include "../../model/enums/AllocationResultStates.h"
#include "../../model/data_models/CompResult/LowCompResult/LowCompResult.h"
#include "../../model/data_models/ComputationDevice/ComputationDevice.h"
#include "../../model/data_models/CompResult/HighCompResult/HighCompResult.h"
#include "../../model/data_models/NetworkCommsModels/HighComplexityAllocation/HighComplexityAllocationComms.h"
#include "../../model/data_models/WorkItems/ProcessingItem/HighProcessingItem/HighProcessingItem.h"
#include "../../model/data_models/NetworkCommsModels/HaltNetworkCommsModel/HaltNetworkCommsModel.h"

namespace services {

    std::pair<std::vector<std::shared_ptr<model::Bucket>>, std::chrono::time_point<std::chrono::system_clock>>
    light_sched_update_network_disc(double bytes_per_millisecond, std::vector<std::shared_ptr<model::Bucket>> disc_link);

    std::pair<uint64_t, uint64_t>
    light_sched_update_bw_vals(const std::vector<double> &bits_per_second_vect, double averageBPS, double jitter);

    std::tuple<ResultState, std::shared_ptr<model::TimeWindow>, std::shared_ptr<model::LowCompResult>>
    light_sched_low_comp_allocation_call(std::string dnn_id, std::string host,
                                         std::chrono::time_point<std::chrono::system_clock> deadline,
                                         std::map<std::string, std::shared_ptr<model::ComputationDevice>> devices,
                                         std::shared_ptr<model::ComputationDevice> device,
                                         bool isReallocation,
                                         double bw_bytes);

    int selectResourceConfig(std::chrono::time_point<std::chrono::system_clock> start,
                             std::chrono::time_point<std::chrono::system_clock> deadline);


    std::vector<int> gatherCommSlots(int task_count, std::vector<std::shared_ptr<model::Bucket>> netlink,
                                     std::chrono::time_point<std::chrono::system_clock> currentTime,
                                     std::chrono::time_point<std::chrono::system_clock> last_time_of_reasoning,
                                     double bytes_per_millisecond
    );

    void findResourceWindow(std::chrono::time_point<std::chrono::system_clock> start_time,
                            std::chrono::time_point<std::chrono::system_clock> deadline,
                            std::shared_ptr<model::ComputationDevice> device, int resourceConfig, int task_count,
                            std::string host,
                            std::map<std::string, std::vector<std::pair<int, std::shared_ptr<model::ResourceWindow>>>> &results,
                            std::mutex &mtx);

    std::pair<std::vector<std::pair<int, std::shared_ptr<model::ResourceWindow>>>, std::vector<std::pair<int, std::shared_ptr<model::ResourceWindow>>>>
    selectResults(
            std::map<std::string, std::vector<std::pair<int, std::shared_ptr<model::ResourceWindow>>>> placement_results,
            std::string sourceHost, int task_count);

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
    );

    std::tuple<int, int, uint64_t> selectResourceUsage(int resourceConfig);

    std::vector<std::tuple<ResultState, std::string, std::shared_ptr<model::HighCompResult>>>
    light_sched_high_comp_allocation_call(bool isReallocation, std::vector<std::string> dnn_ids,
                                          double bw_bytes, std::string sourceHost, std::string dnn_id,
                                          std::vector<std::shared_ptr<model::Bucket>> disc_link,
                                          std::chrono::time_point<std::chrono::system_clock> deadline,
                                          std::chrono::time_point<std::chrono::system_clock> last_time_of_reasoning,
                                          std::map<std::string, std::shared_ptr<model::ComputationDevice>> devices);

    std::shared_ptr<model::ComputationDevice> light_sched_regenerate_res_data_structure(std::string host, std::shared_ptr<model::ComputationDevice> device);
} // services

#endif //CONTROLLER_LIGHTSCHEDULINGFUNCTIONS_H
