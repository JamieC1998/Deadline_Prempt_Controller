//
// Created by Jamie Cotter on 09/02/2023.
//

#include "HaltNetworkCommsModel.h"

model::HaltNetworkCommsModel::HaltNetworkCommsModel(enums::network_comms_types type,
                                                    const std::chrono::time_point<std::chrono::system_clock> &commTime,
                                                    const std::map<std::string, std::string> &versionMap)
        : BaseNetworkCommsModel(type, commTime), version_map(versionMap) {}

const std::map<std::string, std::string> &model::HaltNetworkCommsModel::getVersionMap() const {
    return version_map;
}
