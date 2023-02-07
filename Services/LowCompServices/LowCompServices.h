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

    std::pair<bool, std::shared_ptr<std::pair<std::chrono::time_point<std::chrono::system_clock>, std::chrono::time_point<std::chrono::system_clock>>>>
    allocate_task(std::shared_ptr<model::WorkItem> pItem,
                  std::map<std::string, std::shared_ptr<model::ComputationDevice>> sharedPtr,
                  std::shared_ptr<std::pair<std::chrono::time_point<std::chrono::system_clock>, std::chrono::time_point<std::chrono::system_clock>>> start_time);

} // services

#endif //CONTROLLER_LOWCOMPSERVICES_H
