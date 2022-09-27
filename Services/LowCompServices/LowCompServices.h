//
// Created by jamiec on 9/27/22.
//

#ifndef CONTROLLER_LOWCOMPSERVICES_H
#define CONTROLLER_LOWCOMPSERVICES_H

#include <utility>
#include <ctime>
#include <memory>
#include "../../model/data_models/WorkItems/BaseWorkItem/WorkItem.h"
#include "../../model/data_models/ComputationDevice/ComputationDevice.h"

namespace services {

    std::pair<bool, std::pair<time_t, time_t>> allocate_task(model::WorkItem *pItem,
                                                            std::shared_ptr<model::ComputationDevice> sharedPtr);

} // services

#endif //CONTROLLER_LOWCOMPSERVICES_H
