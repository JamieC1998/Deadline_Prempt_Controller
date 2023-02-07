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
                    std::map<int, std::chrono::time_point<std::chrono::system_clock>> finishTimes,
                    std::string convidx, std::string dnnId);

        const std::map<int, std::chrono::time_point<std::chrono::system_clock>> &getFinishTimes() const;

        void setFinishTimes(const std::map<int, std::chrono::time_point<std::chrono::system_clock>> &finishTimes);

        const std::string &getConvidx() const;

        void setConvidx(const std::string &convidx);

        const std::string &getDnnId() const;

        void setDnnId(const std::string &dnnId);

    private:
        std::map<int, std::chrono::time_point<std::chrono::system_clock>> finish_times;
        std::string convidx;
        std::string dnn_id;
    };

} // model

#endif //CONTROLLER_STATEUPDATE_H
