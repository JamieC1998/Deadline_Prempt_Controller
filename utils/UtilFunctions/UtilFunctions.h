//
// Created by Jamie Cotter on 25/10/2022.
//

#ifndef CONTROLLER_UTILFUNCTIONS_H
#define CONTROLLER_UTILFUNCTIONS_H

#include <string>
#include "chrono"
#include "../../model/data_models/WorkItems/ProcessingItem/ProcessingItem.h"

namespace utils {

    std::string convertDateToString(std::chrono::time_point<std::chrono::system_clock> timePoint);
    unsigned long calculateSizeOfInputData(std::shared_ptr<model::BaseDNNModel> processingItem);

} // utils

#endif //CONTROLLER_UTILFUNCTIONS_H
