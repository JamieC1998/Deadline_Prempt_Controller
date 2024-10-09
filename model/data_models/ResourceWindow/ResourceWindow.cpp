//
// Created by Jamie Cotter on 09/09/2024.
//

#include "ResourceWindow.h"
#include "../../../utils/UtilFunctions/UtilFunctions.h"
#include <sstream>
#include <iostream>
#include <utility>

namespace model {
// Constructor Implementation
    ResourceWindow::ResourceWindow(std::chrono::time_point<std::chrono::system_clock> start,
                                   std::chrono::time_point<std::chrono::system_clock> stop, std::string deviceId, int cap)
            : timeWindow(std::make_shared<TimeWindow>(start, stop)), capacity(cap), deviceId(std::move(deviceId)) {}

    ResourceWindow::ResourceWindow(std::chrono::time_point<std::chrono::system_clock> start,
                                   std::chrono::time_point<std::chrono::system_clock> stop, int cap)
            : timeWindow(std::make_shared<TimeWindow>(start, stop)), capacity(cap) {}

// toString Method Implementation
    std::string ResourceWindow::toString() const {
        std::ostringstream oss;
        oss << "[" << utils::debugTimePointToString(timeWindow->start) << ", " << utils::debugTimePointToString(timeWindow->stop) << "] "
            << "id = " << deviceId << " "
            << "capacity = " << capacity;
        return oss.str();
    }

// Overloading << operator for easy output
    std::ostream &operator<<(std::ostream &os, const ResourceWindow &rw) {
        os << rw.toString();
        return os;
    }
}