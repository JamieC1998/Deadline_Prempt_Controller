//
// Created by Jamie Cotter on 10/11/2023.
//

#include "WorkRequest.h"

namespace model {

    WorkRequest::WorkRequest(const std::shared_ptr<std::vector<std::string>> &hostList, enums::request_type requestType,
                             int reqCounter, std::chrono::time_point<std::chrono::system_clock> tp) : WorkItem(hostList, requestType), request_counter(reqCounter), creation(tp) {
    }

    int WorkRequest::getRequestCounter() const {
        return request_counter;
    }

    const std::chrono::time_point<std::chrono::system_clock> &WorkRequest::getCreation() const {
        return creation;
    }
} // model