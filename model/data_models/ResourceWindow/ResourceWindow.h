//
// Created by Jamie Cotter on 09/09/2024.
//

#ifndef CONTROLLER_RESOURCEWINDOW_H
#define CONTROLLER_RESOURCEWINDOW_H

#include <string>
#include "../TimeWindow/TimeWindow.h"

namespace model {
    class ResourceWindow {
    public:
        std::shared_ptr<TimeWindow> timeWindow;
        int capacity;
        std::string deviceId;
        int track_id;

        // Constructor
        ResourceWindow(std::chrono::time_point<std::chrono::system_clock> start,
                       std::chrono::time_point<std::chrono::system_clock> stop, std::string devId, int cap, int trackId);

        // Constructor
        ResourceWindow(std::chrono::time_point<std::chrono::system_clock> start,
                       std::chrono::time_point<std::chrono::system_clock> stop, int cap);

        // Method to get string representation of the object
        std::string toString() const;

        // Overloading << operator for easy output
        friend std::ostream &operator<<(std::ostream &os, const ResourceWindow &rw);
    };
}

#endif //CONTROLLER_RESOURCEWINDOW_H
