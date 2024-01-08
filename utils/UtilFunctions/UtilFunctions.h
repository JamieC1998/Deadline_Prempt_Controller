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
#include "../../model/data_models/ComputationDevice/ComputationDevice.h"

namespace utils {

    std::string convertDateToString(std::chrono::time_point<std::chrono::high_resolution_clock> timePoint);
    unsigned long calculateStateUpdateSize();
    std::ifstream::pos_type filesize(std::string filename);
    std::string debugTimePointToString(const std::chrono::system_clock::time_point& tp);
    std::pair<int, int> fetchN_M(int position, int width, int height);
    bool is_allocated(std::string task_id, std::vector<std::string> completed_tasks);
    std::map<std::string, int> generateAllocationMap(std::map<std::string, std::shared_ptr<model::ComputationDevice>> devices);

    bool compare_work_items(std::shared_ptr<model::WorkItem> a, std::shared_ptr<model::WorkItem> b);
    std::pair<bool, std::unordered_map<std::string, std::chrono::time_point<std::chrono::system_clock>>> search_and_prune_version(std::string request_version, std::unordered_map<std::string, std::chrono::time_point<std::chrono::system_clock>> &request_version_list);
} // utils

#endif //CONTROLLER_UTILFUNCTIONS_H
