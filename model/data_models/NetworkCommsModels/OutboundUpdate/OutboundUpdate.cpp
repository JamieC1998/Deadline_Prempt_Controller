//
// Created by Jamie Cotter on 07/02/2023.
//

#include "OutboundUpdate.h"

#include <utility>

const std::shared_ptr<model::HighCompResult> &model::OutboundUpdate::getDnn() const {
    return dnn;
}

void model::OutboundUpdate::setDnn(const std::shared_ptr<model::HighCompResult> &dnn) {
    OutboundUpdate::dnn = dnn;
}

model::OutboundUpdate::OutboundUpdate(enums::network_comms_types type,
                                      const std::chrono::time_point<std::chrono::system_clock> &commTime,
                                      std::shared_ptr<model::HighCompResult> dnn,
                                      std::string updateConvidx, uint64_t oldVersion) : BaseNetworkCommsModel(type, commTime),
                                                                          dnn(std::move(dnn)), update_convidx(std::move(updateConvidx)), old_version(oldVersion) {}

const std::string &model::OutboundUpdate::getUpdateConvidx() const {
    return update_convidx;
}

void model::OutboundUpdate::setUpdateConvidx(const std::string &updateConvidx) {
    update_convidx = updateConvidx;
}

uint64_t model::OutboundUpdate::getOldVersion() const {
    return old_version;
}

void model::OutboundUpdate::setOldVersion(uint64_t oldVersion) {
    old_version = oldVersion;
}
