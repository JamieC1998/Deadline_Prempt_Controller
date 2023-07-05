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

        auto pI = std::static_pointer_cast<model::LowProcessingItem>(pItem);
        auto [dnn_id, host] = pI->getDnnIdAndDevice();

        std::chrono::time_point<std::chrono::system_clock> dr = (start_time->second + std::chrono::milliseconds{LOW_COMPLEXITY_PROCESSING_TIME});

        time_window = std::make_shared<std::pair<std::chrono::time_point<std::chrono::system_clock>, std::chrono::time_point<std::chrono::system_clock>>>(
                start_time->second,
                dr);

        return make_pair(services::isValidNode(start_time->second, dr,
                                  sharedPtr[host]), time_window);
    }
} // services