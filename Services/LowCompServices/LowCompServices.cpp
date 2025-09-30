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
    std::pair<bool, std::shared_ptr<model::TimeWindow>>
    allocate_task(std::shared_ptr<model::WorkItem> pItem,
                  std::map<std::string, std::shared_ptr<model::ComputationDevice>> sharedPtr,
                  std::shared_ptr<model::TimeWindow> start_time) {
        std::shared_ptr<model::TimeWindow> time_window;
        std::shared_ptr<std::map<std::string, model::TimeWindow>> result;

        auto pI = std::static_pointer_cast<model::LowProcessingItem>(pItem);
        auto [dnn_id, host] = pI->getDnnIdAndDevice();

        std::chrono::time_point<std::chrono::system_clock> dr = (start_time->stop + std::chrono::milliseconds{LOW_COMPLEXITY_PROCESSING_TIME});

        time_window = std::make_shared<model::TimeWindow>(
                start_time->stop,
                dr);

        return make_pair(services::isValidNode(start_time->stop, dr,
                                  sharedPtr[host]), time_window);
    }
} // services