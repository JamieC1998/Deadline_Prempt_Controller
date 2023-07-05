//
// Created by Jamie Cotter on 28/04/2023.
//

#include "HaltWorkItem.h"

namespace model {
    HaltWorkItem::HaltWorkItem(enums::request_type requestType, const std::string &hostToExamine,
                               const std::chrono::time_point<std::chrono::system_clock> &startTime,
                               const std::chrono::time_point<std::chrono::system_clock> &finTime) : WorkItem(
            requestType), host_to_examine(hostToExamine), start_time(startTime), fin_time(finTime) {}

    const std::string &HaltWorkItem::getHostToExamine() const {
        return host_to_examine;
    }

    const std::chrono::time_point<std::chrono::system_clock> &HaltWorkItem::getStartTime() const {
        return start_time;
    }

    const std::chrono::time_point<std::chrono::system_clock> &HaltWorkItem::getFinTime() const {
        return fin_time;
    }
} // model