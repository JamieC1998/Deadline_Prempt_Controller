//
// Created by Jamie Cotter on 06/02/2023.
//

#include "LowComplexityAllocationComms.h"

#include <utility>

namespace model {
    LowComplexityAllocationComms::LowComplexityAllocationComms(enums::network_comms_types type,
                                                               const std::chrono::time_point<std::chrono::system_clock> &commTime,
                                                               std::shared_ptr<LowCompResult> allocatedTask,
                                                               std::string host) : BaseNetworkCommsModel(type,
                                                                                                                commTime),
                                                                                          allocated_task(std::move(allocatedTask)),
                                                                                          host(std::move(host)) {}

    const std::shared_ptr<LowCompResult> &LowComplexityAllocationComms::getAllocatedTask() const {
        return allocated_task;
    }

    void LowComplexityAllocationComms::setAllocatedTask(const std::shared_ptr<LowCompResult> &allocatedTask) {
        allocated_task = allocatedTask;
    }

    const std::string &LowComplexityAllocationComms::getHost() const {
        return host;
    }

    void LowComplexityAllocationComms::setHost(const std::string &host) {
        LowComplexityAllocationComms::host = host;
    }
} // model