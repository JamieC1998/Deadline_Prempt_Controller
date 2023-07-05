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
    findNode(std::map<std::string, std::shared_ptr<model::ComputationDevice>> &devices,
             std::chrono::time_point<std::chrono::system_clock> src_expected_start,
             std::chrono::time_point<std::chrono::system_clock> src_expected_finish,
             std::chrono::time_point<std::chrono::system_clock> remote_expected_start,
             std::chrono::time_point<std::chrono::system_clock> remote_expected_finish,
             const std::string &srcHost, int core_usage, std::map<std::string, int> map,
             const std::string string);

    int calculateDeviceCoreUsage(std::chrono::time_point<std::chrono::system_clock> expected_start,
                                 std::chrono::time_point<std::chrono::system_clock> expected_finish,
                                 const std::shared_ptr<model::ComputationDevice> &device, const std::string string);

} // services

#endif //CONTROLLER_HIGHCOMPSERVICES_H
