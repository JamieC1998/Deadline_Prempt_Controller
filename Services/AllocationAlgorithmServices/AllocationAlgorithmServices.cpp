//
// Created by jamiec on 9/27/22.
//

#include "AllocationAlgorithmServices.h"

namespace services {
    bool isValidNode(std::chrono::time_point<std::chrono::system_clock> start_time, std::chrono::time_point<std::chrono::system_clock> finish_time,
                     std::shared_ptr<model::ComputationDevice> device){

        int core_usage = 0;
        for (auto &task: device->DNNS) {
            /* If we are at the end of the task list */

            if ((max(start_time, task->getEstimatedStart()) -
                 min(finish_time, task->getEstimatedFinish())).count() <= 0)
                core_usage += task->getCoreAllocation();
        }

        return checkCapacity(core_usage + 1, device);
    }

    bool checkCapacity(int cpu_usage, const std::shared_ptr<model::ComputationDevice> &node) {

        if (cpu_usage > node->getCores())
            return false;
        return true;
    }
} // services