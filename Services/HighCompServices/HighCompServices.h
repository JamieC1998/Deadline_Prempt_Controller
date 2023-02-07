//
// Created by jamiec on 11/9/22.
//

#ifndef CONTROLLER_HIGHCOMPSERVICES_H
#define CONTROLLER_HIGHCOMPSERVICES_H

#include <chrono>
#include <map>
#include "../../model/data_models/ComputationDevice/ComputationDevice.h"

namespace services {

    //Result contains selected device id and host name
    //If no device found, -1
    std::pair<int, std::string>
    findNode(
            std::map<std::string, std::shared_ptr<model::ComputationDevice>> &devices,
            std::map<int, std::vector<std::shared_ptr<model::Task>>> temp_task_list,
            std::chrono::time_point<std::chrono::system_clock> expected_start,
            std::chrono::time_point<std::chrono::system_clock> expected_finish);

} // services

#endif //CONTROLLER_HIGHCOMPSERVICES_H
