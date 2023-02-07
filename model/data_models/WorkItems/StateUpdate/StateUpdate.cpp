//
// Created by jamiec on 10/4/22.
//

#include "StateUpdate.h"

#include <utility>

namespace model {

    StateUpdate::StateUpdate(const std::shared_ptr<std::vector<std::string>> &hostList, enums::request_type requestType,
                             std::map<int, std::chrono::time_point<std::chrono::system_clock>> finishTimes,
                             std::string convidx, std::string dnnId) : WorkItem(hostList, requestType),
                                                                                     finish_times(std::move(finishTimes)),
                                                                                     convidx(std::move(convidx)), dnn_id(std::move(dnnId)) {}

    const std::map<int, std::chrono::time_point<std::chrono::system_clock>> &StateUpdate::getFinishTimes() const {
        return finish_times;
    }

    void
    StateUpdate::setFinishTimes(const std::map<int, std::chrono::time_point<std::chrono::system_clock>> &finishTimes) {
        finish_times = finishTimes;
    }

    const std::string &StateUpdate::getConvidx() const {
        return convidx;
    }

    void StateUpdate::setConvidx(const std::string &convidx) {
        StateUpdate::convidx = convidx;
    }

    const std::string &StateUpdate::getDnnId() const {
        return dnn_id;
    }

    void StateUpdate::setDnnId(const std::string &dnnId) {
        dnn_id = dnnId;
    }


} // model