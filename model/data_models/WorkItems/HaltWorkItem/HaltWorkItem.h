//
// Created by Jamie Cotter on 28/04/2023.
//

#ifndef CONTROLLER_HALTWORKITEM_H
#define CONTROLLER_HALTWORKITEM_H

#include "../BaseWorkItem/WorkItem.h"

namespace model {

    class HaltWorkItem: public WorkItem{
    public:
        HaltWorkItem(enums::request_type requestType, const std::string &hostToExamine,
                     const std::chrono::time_point<std::chrono::system_clock> &startTime,
                     const std::chrono::time_point<std::chrono::system_clock> &finTime);

        const std::string &getHostToExamine() const;

        const std::chrono::time_point<std::chrono::system_clock> &getStartTime() const;

        const std::chrono::time_point<std::chrono::system_clock> &getFinTime() const;

    private:
        std::string host_to_examine;
        std::chrono::time_point<std::chrono::system_clock> start_time;
        std::chrono::time_point<std::chrono::system_clock> fin_time;
    };

} // model

#endif //CONTROLLER_HALTWORKITEM_H
