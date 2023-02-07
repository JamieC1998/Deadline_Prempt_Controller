//
// Created by Jamie Cotter on 25/10/2022.
//

#ifndef CONTROLLER_UTILFUNCTIONS_H
#define CONTROLLER_UTILFUNCTIONS_H

#include <string>
#include "fstream"
#include "chrono"
#include "../../model/data_models/WorkItems/ProcessingItem/HighProcessingItem/HighProcessingItem.h"
#include "../../model/data_models/LinkAct/LinkAct.h"
#include "../../model/data_models/FTP_Lookup/FTP_Lookup.h"

namespace utils {

    std::string convertDateToString(std::chrono::time_point<std::chrono::high_resolution_clock> timePoint);
    uint64_t calculateSizeOfInputData(std::shared_ptr<model::FTP_Lookup> lookup_table);
    unsigned long calculateStateUpdateSize();
    std::ifstream::pos_type filesize(std::string filename);
    std::shared_ptr<model::FTP_Lookup> parseDNN();

    std::pair<int, int> fetchN_M(int position, int width, int height);
} // utils

#endif //CONTROLLER_UTILFUNCTIONS_H
