//
// Created by Jamie Cotter on 09/09/2024.
//

#ifndef CONTROLLER_TIMEWINDOW_H
#define CONTROLLER_TIMEWINDOW_H

#include <string>
#include <cpprest/json.h>

namespace model {
    class TimeWindow {
    public:
        std::chrono::time_point<std::chrono::system_clock> start;
        std::chrono::time_point<std::chrono::system_clock> stop;

        // Constructor
        TimeWindow(std::chrono::time_point<std::chrono::system_clock> start, std::chrono::time_point<std::chrono::system_clock> stop);

        // Method to convert the TimeWindow to a string representation
        std::string toString() const;
        web::json::value convertToJson();
    };
}

#endif //CONTROLLER_TIMEWINDOW_H
