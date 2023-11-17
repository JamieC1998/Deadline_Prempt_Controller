//
// Created by Jamie Cotter on 12/11/2023.
//

#include "WorkRequestResponse.h"

namespace model {
    WorkRequestResponse::WorkRequestResponse(enums::network_comms_types type,
                                             const std::chrono::time_point<std::chrono::system_clock> &commTime,
                                             bool isSuccess, int coreCapacity, std::string contactDevice)
            : BaseNetworkCommsModel(type, commTime),
              is_success(isSuccess),
              core_capacity(coreCapacity), contact_device(contactDevice) {}

    const std::string &WorkRequestResponse::getContactDevice() const {
        return contact_device;
    }

    bool WorkRequestResponse::isSuccess() const {
        return is_success;
    }

    int WorkRequestResponse::getCoreCapacity() const {
        return core_capacity;
    }
} // model