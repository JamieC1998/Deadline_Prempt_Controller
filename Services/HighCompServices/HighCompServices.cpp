//
// Created by jamiec on 11/9/22.
//

#include "HighCompServices.h"
#include "../../utils/UtilFunctions/UtilFunctions.h"

namespace services {
    std::pair<int, std::string>
    findNode(std::map<std::string, std::shared_ptr<model::ComputationDevice>> &devices,
             std::chrono::time_point<std::chrono::system_clock> src_expected_start,
             std::chrono::time_point<std::chrono::system_clock> src_expected_finish,
             std::chrono::time_point<std::chrono::system_clock> remote_expected_start,
             std::chrono::time_point<std::chrono::system_clock> remote_expected_finish,
             const std::string &srcHost, int core_requirement, std::map<std::string, int> allocation_map,
             const std::string dnn_id) {

        int selected_device = -1;
        std::string host_name;

        /* We first check availability on the source device */
        int core_usage = calculateDeviceCoreUsage(src_expected_start, src_expected_finish, devices[srcHost],
                                                  dnn_id);

        if (core_usage + core_requirement <= devices[srcHost]->getCores()) {
            selected_device = devices[srcHost]->getId();
            host_name = devices[srcHost]->getHostName();
        } else {
            // Define a lambda function as a custom comparator
            auto cmp = [](const std::pair<std::string, int> &lhs, const std::pair<std::string, int> &rhs) {
                return lhs.second < rhs.second;
            };

            // Create a vector of map elements
            std::vector<std::pair<std::string, int>> sortedVector(allocation_map.begin(), allocation_map.end());

            std::sort(sortedVector.begin(), sortedVector.end(), cmp);

            for (auto [device_id, usage]: sortedVector) {
                auto device = devices[device_id];
                if (device_id == srcHost)
                    continue;

                core_usage = calculateDeviceCoreUsage(remote_expected_start, remote_expected_finish, device,
                                                      dnn_id);

                if (core_usage + core_requirement <= device->getCores()) {
                    selected_device = device->getId();
                    host_name = device->getHostName();
                    break;
                }
            }
        }
        return std::make_pair(selected_device, host_name);
    }

    int calculateDeviceCoreUsage(std::chrono::time_point<std::chrono::system_clock> expected_start,
                                 std::chrono::time_point<std::chrono::system_clock> expected_finish,
                                 const std::shared_ptr<model::ComputationDevice> &device, const std::string dnn_id) {

        int core_usage = 0;
        for (auto &task: device->DNNS) {
            if ((max(expected_start, task->getEstimatedStart()) -
                 min(expected_finish, task->getEstimatedFinish())).count() <= 0 && dnn_id != task->getDnnId())
                core_usage += task->getCoreAllocation();
        }

        return core_usage;
    }
} // services