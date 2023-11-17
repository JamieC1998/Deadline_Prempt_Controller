//
// Created by Jamie Cotter on 10/11/2023.
//

#include "WorkRequest.h"

namespace model {
    WorkRequest::WorkRequest(const std::shared_ptr<std::vector<std::string>> &hostList, enums::request_type requestType,
                             int capacity) : WorkItem(hostList, requestType), capacity(capacity) {}

    int WorkRequest::getCapacity() const {
        return capacity;
    }

    void WorkRequest::setCapacity(int capacity) {
        WorkRequest::capacity = capacity;
    }
} // model