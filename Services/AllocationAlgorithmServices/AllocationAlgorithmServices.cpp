//
// Created by jamiec on 9/27/22.
//

#include <c++/9/algorithm>
#include "AllocationAlgorithmServices.h"

namespace services {
    bool isValidNode(std::chrono::milliseconds estimatedProcTime, float ram_req,
                     float storage_req, std::shared_ptr<std::pair<std::chrono::time_point<std::chrono::high_resolution_clock>, std::chrono::time_point<std::chrono::high_resolution_clock>>> startPoint,
                     std::shared_ptr<model::ComputationDevice> device){

        auto estimatedFinish = estimatedProcTime + startPoint->second;
        struct ResourceEvent {
            int gpu_usage;
            float ram_usage;
            float storage;

            bool is_increase;
            std::chrono::time_point<std::chrono::high_resolution_clock, std::chrono::milliseconds> time;
        };

        std::vector<ResourceEvent> resource_events;

        for (const auto &item: device->getTasks()) {
            if (startPoint->second <= item->getEstimatedStart() && item->getEstimatedStart() <= estimatedFinish) {
                for (int i = 0; i < 2; i++) {
                    resource_events.push_back(
                            {1, ram_req, storage_req, static_cast<bool>(i),
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

        float max_ram_usage = 0;
        float max_storage_usage = 0;
        int max_core_usage = 0;

        float current_ram_usage = 0;
        float current_storage_usage = 0;
        int current_core_usage = 0;

        for (const auto &resource_event: resource_events) {
            if (resource_event.is_increase) {
                current_core_usage += resource_event.gpu_usage;
                current_storage_usage += resource_event.storage;
                current_ram_usage += resource_event.ram_usage;

                if (current_core_usage > max_core_usage)
                    max_core_usage = current_core_usage;
                if (current_ram_usage > max_ram_usage)
                    max_ram_usage = current_ram_usage;
                if (current_storage_usage > max_storage_usage)
                    max_storage_usage = current_storage_usage;
            } else {
                current_core_usage -= resource_event.gpu_usage;
                current_storage_usage -= resource_event.storage;
                current_ram_usage -= resource_event.ram_usage;
            }
        }

        if (!services::checkCapacity((max_ram_usage + ram_req), max_core_usage + 1,
                                            max_storage_usage + storage_req, device))
            return false;
        return true;
    }

    bool services::checkCapacity(float ram_usage, int gpu_usage, float storage_usage,
                                        const std::shared_ptr<model::ComputationDevice> &node) {

        if (ram_usage > node->getRam())
            return false;
        if (storage_usage > node->getStorage() * 1024)
            return false;
        if (gpu_usage > node->getCores())
            return false;
        return true;
    }
} // services