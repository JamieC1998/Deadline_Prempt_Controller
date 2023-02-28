//
// Created by jamiec on 11/9/22.
//

#include "HighCompServices.h"

namespace services {
    std::pair<int, std::string>
    findNode(
            std::map<std::string, std::shared_ptr<model::ComputationDevice>> &devices,
            std::map<int, std::vector<std::shared_ptr<model::Task>>> temp_task_list,
            std::chrono::time_point<std::chrono::system_clock> expected_start,
            std::chrono::time_point<std::chrono::system_clock> expected_finish) {

        int selected_device = -1;
        std::string host_name;

        for (auto [device_id, device]: devices) {
            std::vector<std::shared_ptr<model::Task>> overlapping_tasks;
            std::vector<std::shared_ptr<model::Task>> &temp_tasks = temp_task_list[device->getId()];
            for (auto &temp_task: temp_tasks) {
                /* If we are at the end of the task list */
                if ((max(expected_start, temp_task->getEstimatedStart()) -
                     min(expected_finish, temp_task->getEstimatedFinish())).count() <= 0)
                    overlapping_tasks.push_back(temp_task);
            }

            if (overlapping_tasks.size() < device->getCores() - 1) {
                selected_device = device->getId();
                host_name = device->getHostName();
                break;
            }
        }
        return std::make_pair(selected_device, host_name);
    }
} // services