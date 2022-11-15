//
// Created by jamiec on 9/27/22.
//

#ifndef CONTROLLER_LOWCOMPSERVICES_H
#define CONTROLLER_LOWCOMPSERVICES_H

#include <utility>
#include <ctime>
#include <memory>
#include <map>
#include "../../model/data_models/WorkItems/BaseWorkItem/WorkItem.h"
#include "../../model/data_models/ComputationDevice/ComputationDevice.h"

namespace services {

    std::pair<bool, std::shared_ptr<std::map<std::string, std::pair<std::chrono::time_point<std::chrono::high_resolution_clock>, std::chrono::time_point<std::chrono::high_resolution_clock>>>>>
    allocate_task(model::WorkItem *pItem,
                  std::map<std::string, std::shared_ptr<model::ComputationDevice>> sharedPtr,
                  std::map<std::string, std::shared_ptr<std::pair<std::chrono::time_point<std::chrono::high_resolution_clock>, std::chrono::time_point<std::chrono::high_resolution_clock>>>> start_time);

} // services

#endif //CONTROLLER_LOWCOMPSERVICES_H
