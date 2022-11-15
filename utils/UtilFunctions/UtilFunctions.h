//
// Created by Jamie Cotter on 25/10/2022.
//

#ifndef CONTROLLER_UTILFUNCTIONS_H
#define CONTROLLER_UTILFUNCTIONS_H

#include <string>
#include "fstream"
#include "chrono"
#include "../../model/data_models/WorkItems/ProcessingItem/ProcessingItem.h"
#include "../../model/data_models/LinkAct/LinkAct.h"

namespace utils {

    std::string convertDateToString(std::chrono::time_point<std::chrono::high_resolution_clock> timePoint);
    unsigned long calculateSizeOfInputData(std::shared_ptr<model::BaseDNNModel> processingItem);
    unsigned long calculateStateUpdateSize();
    std::ifstream::pos_type filesize(std::string filename);
} // utils

#endif //CONTROLLER_UTILFUNCTIONS_H
