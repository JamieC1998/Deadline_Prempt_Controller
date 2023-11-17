//
// Created by Jamie Cotter on 04/02/2023.
//

#include "HighComplexityAllocationComms.h"

#include <utility>

namespace model {
    HighComplexityAllocationComms::HighComplexityAllocationComms(enums::network_comms_types type,
                                                                 const std::chrono::time_point<std::chrono::system_clock> &commTime,
                                                                 std::shared_ptr<HighCompResult> allocatedTask,
                                                                 std::string host, bool isSuccess)
            : BaseNetworkCommsModel(type,
                                    commTime),
              allocatedTask(std::move(
                      allocatedTask)),
              host(std::move(host)),
              success(isSuccess) {}


    HighComplexityAllocationComms::HighComplexityAllocationComms(enums::network_comms_types type,
                                                                 const std::chrono::time_point<std::chrono::system_clock> &commTime,
                                                                 bool isSuccess, std::string host)
            : BaseNetworkCommsModel(type, commTime), success(isSuccess), host(host) {

    }

    const std::shared_ptr<HighCompResult> &HighComplexityAllocationComms::getAllocatedTask() const {
        return allocatedTask;
    }

    void HighComplexityAllocationComms::setAllocatedTask(const std::shared_ptr<HighCompResult> &allocatedTask) {
        HighComplexityAllocationComms::allocatedTask = allocatedTask;
    }

    const std::string &HighComplexityAllocationComms::getHost() const {
        return host;
    }

    void HighComplexityAllocationComms::setHost(const std::string &host) {
        HighComplexityAllocationComms::host = host;
    }

    bool HighComplexityAllocationComms::isSuccess() const {
        return success;
    }

} // model