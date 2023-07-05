//
// Created by Jamie Cotter on 09/02/2023.
//

#include "HaltNetworkCommsModel.h"

#include <utility>

model::HaltNetworkCommsModel::HaltNetworkCommsModel(enums::network_comms_types type,
                                                    const std::chrono::time_point<std::chrono::system_clock> &commTime,
                                                    const std::string &hostToContact, const std::string &dnnId,
                                                    uint64_t versionNumber) : BaseNetworkCommsModel(type, commTime),
                                                                              hostToContact(hostToContact),
                                                                              dnnId(dnnId),
                                                                              versionNumber(versionNumber) {}

const std::string &model::HaltNetworkCommsModel::getHostToContact() const {
    return hostToContact;
}

const std::string &model::HaltNetworkCommsModel::getDnnId() const {
    return dnnId;
}

uint64_t model::HaltNetworkCommsModel::getVersionNumber() const {
    return versionNumber;
}
