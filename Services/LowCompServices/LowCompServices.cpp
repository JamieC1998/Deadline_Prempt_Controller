//
// Created by jamiec on 9/27/22.
//

#include <chrono>
#include "LowCompServices.h"
#include "../../model/data_models/WorkItems/ProcessingItem/HighProcessingItem/HighProcessingItem.h"
#include "../AllocationAlgorithmServices/AllocationAlgorithmServices.h"
#include "../../model/data_models/WorkItems/ProcessingItem/LowProcessingItem/LowProcessingItem.h"
#include "../../Constants/AllocationMacros.h"

using namespace constant;
using namespace std::chrono_literals;

namespace services {

    /* Given a set of low complexity DNNs and a minimum start time for each it attempts to find a placement on their
     * host before their deadline */
    std::pair<bool, std::shared_ptr<std::pair<std::chrono::time_point<std::chrono::system_clock>, std::chrono::time_point<std::chrono::system_clock>>>>
    allocate_task(std::shared_ptr<model::WorkItem> pItem,
                  std::map<std::string, std::shared_ptr<model::ComputationDevice>> sharedPtr,
                  std::shared_ptr<std::pair<std::chrono::time_point<std::chrono::system_clock>, std::chrono::time_point<std::chrono::system_clock>>> start_time) {
        std::shared_ptr<std::pair<std::chrono::time_point<std::chrono::system_clock>, std::chrono::time_point<std::chrono::system_clock>>> time_window;
        std::shared_ptr<std::map<std::string, std::pair<std::chrono::time_point<std::chrono::system_clock>, std::chrono::time_point<std::chrono::system_clock>>>> result;

        auto pI = static_pointer_cast<model::LowProcessingItem>(pItem);
        auto [dnn_id, host] = pI->getDnnIdAndDevice();

        std::chrono::system_clock::duration dr =
                (start_time->first + std::chrono::milliseconds{LOW_COMPLEXITY_PROCESSING_TIME}) - start_time->first;

        if (services::isValidNode(std::chrono::duration_cast<std::chrono::milliseconds>(dr), start_time,
                                  sharedPtr[host])) {
            time_window = std::make_shared<std::pair<std::chrono::time_point<std::chrono::system_clock>, std::chrono::time_point<std::chrono::system_clock>>>(
                    start_time->first,
                    std::chrono::time_point_cast<std::chrono::milliseconds>(
                            start_time->first +
                            std::chrono::milliseconds{LOW_COMPLEXITY_PROCESSING_TIME}));

            return make_pair(true, time_window);
        }

        return std::make_pair(false, time_window);
    }
} // services