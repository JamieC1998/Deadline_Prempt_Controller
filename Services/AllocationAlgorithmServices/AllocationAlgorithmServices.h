//
// Created by jamiec on 9/27/22.
//

#ifndef CONTROLLER_ALLOCATIONALGORITHMSERVICES_H
#define CONTROLLER_ALLOCATIONALGORITHMSERVICES_H

#include <chrono>
#include <vector>
#include <memory>
#include "../../model/data_models/ComputationDevice/ComputationDevice.h"

namespace services {
    bool isValidNode(std::chrono::milliseconds estimatedProcTime, std::shared_ptr<std::pair<std::chrono::time_point<std::chrono::system_clock>, std::chrono::time_point<std::chrono::system_clock>>> point,
                     std::shared_ptr<model::ComputationDevice> device);

    bool checkCapacity(int gpu_usage,
                  const std::shared_ptr<model::ComputationDevice> &node);
} // services

#endif //CONTROLLER_ALLOCATIONALGORITHMSERVICES_H
