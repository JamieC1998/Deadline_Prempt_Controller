//
// Created by jamiec on 10/4/22.
//

#ifndef CONTROLLER_STATEUPDATE_H
#define CONTROLLER_STATEUPDATE_H

#include <chrono>
#include <map>
#include "../BaseWorkItem/WorkItem.h"

namespace model {

    class StateUpdate : public WorkItem {
    public:
        StateUpdate(const std::shared_ptr<std::vector<std::string>> &hostList, enums::request_type requestType,
                    std::chrono::time_point<std::chrono::system_clock> finishTime, std::string dnnId);

        const std::chrono::time_point<std::chrono::system_clock> &getFinishTime() const;

        void setFinishTime(const std::chrono::time_point<std::chrono::system_clock> &finishTime);

        const std::string &getDnnId() const;

        void setDnnId(const std::string &dnnId);

        bool isSuccess() const;

        void setSuccess(bool success);

    private:
        std::chrono::time_point<std::chrono::system_clock> finish_time;
        std::string dnn_id;
        bool success = true;
    };

} // model

#endif //CONTROLLER_STATEUPDATE_H
