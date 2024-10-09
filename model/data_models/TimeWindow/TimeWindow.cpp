//
// Created by Jamie Cotter on 09/09/2024.
//

#include "TimeWindow.h"
#include "../../../utils/UtilFunctions/UtilFunctions.h"
#include <sstream>

namespace model {
// Constructor Implementation
    TimeWindow::TimeWindow(std::chrono::time_point<std::chrono::system_clock> start,
                           std::chrono::time_point<std::chrono::system_clock> stop)
            : start(start), stop(stop) {}


    web::json::value TimeWindow::convertToJson() {
        web::json::value result;

        result["start"] = TimeWindow::start.time_since_epoch().count();
        result["stop"] = TimeWindow::stop.time_since_epoch().count();

        return result;
    }

}

// toString Method Implementation
std::string model::TimeWindow::toString() const {
    std::ostringstream oss;
    oss << "[" << utils::debugTimePointToString(start) << ", " << utils::debugTimePointToString(stop) << "]";
    return oss.str();
}

