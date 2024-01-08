//
// Created by Jamie Cotter on 10/11/2023.
//

#ifndef CONTROLLER_WORKREQUEST_H
#define CONTROLLER_WORKREQUEST_H

#include "../BaseWorkItem/WorkItem.h"

namespace model {

    class WorkRequest: public WorkItem{
    public:
        WorkRequest(const std::shared_ptr<std::vector<std::string>> &hostList, enums::request_type requestType, int reqCounter, std::chrono::time_point<std::chrono::system_clock> tp);

        int getRequestCounter() const;

        const std::chrono::time_point<std::chrono::system_clock> &getCreation() const;

    private:
        int request_counter = -1;
        std::chrono::time_point<std::chrono::system_clock> creation;
    };
} // model

#endif //CONTROLLER_WORKREQUEST_H
