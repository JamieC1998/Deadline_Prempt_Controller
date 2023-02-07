//
// Created by jamiec on 9/27/22.
//

#include "AllocationAlgorithmServices.h"

namespace services {
    bool isValidNode(std::chrono::milliseconds estimatedProcTime, std::shared_ptr<std::pair<std::chrono::time_point<std::chrono::system_clock>, std::chrono::time_point<std::chrono::system_clock>>> startPoint,
                     std::shared_ptr<model::ComputationDevice> device){

        auto estimatedFinish = estimatedProcTime + startPoint->first;
        struct ResourceEvent {
            int gpu_usage;

            bool is_increase;
            std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds> time;
        };

        std::vector<ResourceEvent> resource_events;

        for (const auto &item: device->getTasks()) {
            if (startPoint->second <= item->getEstimatedStart() && item->getEstimatedStart() <= estimatedFinish) {
                for (int i = 0; i < 2; i++) {
                    resource_events.push_back(
                            {1, static_cast<bool>(i),
                             std::chrono::time_point_cast<std::chrono::milliseconds>(static_cast<bool>(i) ? (item->getEstimatedStart() < startPoint->second)
                                                    ? startPoint->second
                                                    : item->getEstimatedStart()
                                                  : (item->getEstimatedFinish() > estimatedFinish)
                                                    ? estimatedFinish
                                                    : item->getEstimatedFinish())});
                }
            }
        }

        std::sort(std::begin(resource_events), std::end(resource_events),
             [](const ResourceEvent &a, const ResourceEvent &b) -> bool {
                 return (a.time == b.time) ? a.is_increase : a.time < b.time;
             });

        int max_core_usage = 0;

        int current_core_usage = 0;

        for (const auto &resource_event: resource_events) {
            if (resource_event.is_increase) {
                current_core_usage += resource_event.gpu_usage;

                if (current_core_usage > max_core_usage)
                    max_core_usage = current_core_usage;
            } else {
                current_core_usage -= resource_event.gpu_usage;
            }
        }

        if (!services::checkCapacity(max_core_usage + 1, device))
            return false;
        return true;
    }

    bool services::checkCapacity(int gpu_usage, const std::shared_ptr<model::ComputationDevice> &node) {

        if (gpu_usage > node->getCores())
            return false;
        return true;
    }
} // services