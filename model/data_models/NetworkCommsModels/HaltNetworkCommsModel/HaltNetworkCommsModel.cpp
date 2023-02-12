//
// Created by Jamie Cotter on 09/02/2023.
//

#include "HaltNetworkCommsModel.h"

#include <utility>

model::HaltNetworkCommsModel::HaltNetworkCommsModel(enums::network_comms_types type,
                                                    const std::chrono::time_point<std::chrono::system_clock> &commTime,
                                                    std::map<std::string, uint64_t> versionMap)
        : BaseNetworkCommsModel(type, commTime), version_map(std::move(versionMap)) {}

const std::map<std::string, uint64_t> &model::HaltNetworkCommsModel::getVersionMap() const {
    return version_map;
}
