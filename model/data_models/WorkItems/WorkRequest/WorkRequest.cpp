//
// Created by Jamie Cotter on 10/11/2023.
//

#include "WorkRequest.h"

namespace model {

    WorkRequest::WorkRequest(const std::shared_ptr<std::vector<std::string>> &hostList, enums::request_type requestType,
                             int l_capacity,
                             int reqCounter, std::chrono::time_point<std::chrono::system_clock> tp) : WorkItem(hostList,
                                                                                                               requestType),
                                                                                                      capacity(
                                                                                                              l_capacity),
                                                                                                      request_counter(
                                                                                                              reqCounter),
                                                                                                      creation(tp) {
    }

    int WorkRequest::getRequestCounter() const {
        return request_counter;
    }

    const std::chrono::time_point<std::chrono::system_clock> &WorkRequest::getCreation() const {
        return creation;
    }

    int WorkRequest::getCapacity() const {
        return capacity;
    }
} // model