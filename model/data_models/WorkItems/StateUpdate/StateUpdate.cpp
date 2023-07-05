//
// Created by jamiec on 10/4/22.
//

#include "StateUpdate.h"

#include <utility>

namespace model {

    StateUpdate::StateUpdate(const std::shared_ptr<std::vector<std::string>> &hostList, enums::request_type requestType,
                             std::chrono::time_point<std::chrono::system_clock> finishTime,
                             std::string dnnId) : WorkItem(hostList, requestType),
                                                  finish_time(finishTime),
                                                  dnn_id(std::move(dnnId)) {}

    const std::chrono::time_point<std::chrono::system_clock> &StateUpdate::getFinishTime() const {
        return finish_time;
    }

    void StateUpdate::setFinishTime(const std::chrono::time_point<std::chrono::system_clock> &finishTime) {
        finish_time = finishTime;
    }

    const std::string &StateUpdate::getDnnId() const {
        return dnn_id;
    }

    void StateUpdate::setDnnId(const std::string &dnnId) {
        dnn_id = dnnId;
    }

    bool StateUpdate::isSuccess() const {
        return success;
    }

    void StateUpdate::setSuccess(bool success) {
        StateUpdate::success = success;
    }


} // model