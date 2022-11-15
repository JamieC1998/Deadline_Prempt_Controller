//
// Created by jamiec on 10/11/22.
//

#include "BaseNetworkCommsModel.h"

model::BaseNetworkCommsModel::BaseNetworkCommsModel(const std::vector<std::string> &hosts,
                                                    enums::network_comms_types type,
                                                    const std::chrono::time_point<std::chrono::high_resolution_clock> &commTime,
                                                    std::shared_ptr<BaseResult> allocatedTask, std::string input_file)
        : hosts(hosts), type(type), comm_time(commTime), allocatedTask(allocatedTask), input_file_path(input_file) {}

const std::vector<std::string> &model::BaseNetworkCommsModel::getHosts() const {
    return hosts;
}

enums::network_comms_types model::BaseNetworkCommsModel::getType() const {
    return type;
}

const std::chrono::time_point<std::chrono::high_resolution_clock> &model::BaseNetworkCommsModel::getCommTime() const {
    return comm_time;
}

const std::shared_ptr<model::BaseResult> &model::BaseNetworkCommsModel::getAllocatedTask() const {
    return allocatedTask;
}

void model::BaseNetworkCommsModel::setAllocatedTask(const std::shared_ptr<model::BaseResult> &allocatedTask) {
    BaseNetworkCommsModel::allocatedTask = allocatedTask;
}

const std::string &model::BaseNetworkCommsModel::getInputFilePath() const {
    return input_file_path;
}
