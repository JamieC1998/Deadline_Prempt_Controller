//
// Created by jamiec on 9/27/22.
//

#include <chrono>
#include "LowCompServices.h"
#include "../../model/data_models/WorkItems/ProcessingItem/ProcessingItem.h"
#include "../AllocationAlgorithmServices/AllocationAlgorithmServices.h"

using namespace constant;
using namespace std::chrono_literals;

namespace services {

    /* Given a set of low complexity DNNs and a minimum start time for each it attempts to find a placement on their
     * host before their deadline */
    std::pair<bool, std::shared_ptr<std::map<std::string, std::pair<std::chrono::time_point<std::chrono::high_resolution_clock>, std::chrono::time_point<std::chrono::high_resolution_clock>>>>>
    allocate_task(model::WorkItem *pItem,
                  std::map<std::string, std::shared_ptr<model::ComputationDevice>> sharedPtr,
                  std::map<std::string, std::shared_ptr<std::pair<std::chrono::time_point<std::chrono::high_resolution_clock>, std::chrono::time_point<std::chrono::high_resolution_clock>>>> start_time) {

        std::shared_ptr<std::map<std::string, std::pair<std::chrono::time_point<std::chrono::high_resolution_clock>, std::chrono::time_point<std::chrono::high_resolution_clock>>>> result;

        auto *pI = reinterpret_cast<model::ProcessingItem *>(pItem);

        for (std::pair<const std::basic_string<char>, std::shared_ptr<model::BaseDNNModel>> item: pI->getAllocationInputData()) {
            std::shared_ptr<model::BaseDNNModel> base = item.second;

            std::shared_ptr<std::pair<std::chrono::time_point<std::chrono::high_resolution_clock>, std::chrono::time_point<std::chrono::high_resolution_clock>>> estimated_start_time = start_time[item.first];

            std::chrono::high_resolution_clock::duration dr = base->getEstimatedProcTime()[0];

            if (services::isValidNode(std::chrono::duration_cast<std::chrono::milliseconds>(dr), base->getRamReq()[0],
                                      base->getStorageReq()[0], estimated_start_time, sharedPtr[item.first])) {
                result->at(item.first) = std::make_pair(estimated_start_time->second,
                                                        std::chrono::time_point_cast<std::chrono::milliseconds>(
                                                                estimated_start_time->second +
                                                                base->getEstimatedProcTime()[0]));
            } else {
                result->clear();
                return make_pair(false, result);
            }
        }

        return std::make_pair(true, result);
    }
} // services