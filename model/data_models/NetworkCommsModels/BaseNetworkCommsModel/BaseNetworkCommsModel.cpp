//
// Created by jamiec on 10/11/22.
//

#include "BaseNetworkCommsModel.h"

#include <utility>

model::BaseNetworkCommsModel::BaseNetworkCommsModel(enums::network_comms_types type,
                                                    const std::chrono::time_point<std::chrono::system_clock> &commTime)
        : type(type), comm_time(commTime) {}

enums::network_comms_types model::BaseNetworkCommsModel::getType() const {
    return type;
}

const std::chrono::time_point<std::chrono::system_clock> &model::BaseNetworkCommsModel::getCommTime() const {
    return comm_time;
}
