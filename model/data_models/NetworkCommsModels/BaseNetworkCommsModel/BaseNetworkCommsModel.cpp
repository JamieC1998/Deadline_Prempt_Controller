//
// Created by jamiec on 10/11/22.
//

#include "BaseNetworkCommsModel.h"

model::BaseNetworkCommsModel::BaseNetworkCommsModel(const std::vector<std::string> &hosts,
                                                    enums::network_comms_types type,
                                                    const std::chrono::time_point<std::chrono::system_clock> &commTime)
        : hosts(hosts), type(type), comm_time(commTime) {}

const std::vector<std::string> &model::BaseNetworkCommsModel::getHosts() const {
    return hosts;
}

enums::network_comms_types model::BaseNetworkCommsModel::getType() const {
    return type;
}

const std::chrono::time_point<std::chrono::system_clock> &model::BaseNetworkCommsModel::getCommTime() const {
    return comm_time;
}
