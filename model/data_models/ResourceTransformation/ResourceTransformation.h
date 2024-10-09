//
// Created by Jamie Cotter on 09/09/2024.
//

#ifndef CONTROLLER_RESOURCETRANSFORMATION_H
#define CONTROLLER_RESOURCETRANSFORMATION_H
#include "chrono"

namespace model {
    class ResourceTransformation {
    public:
        bool isIncrease;
        std::chrono::time_point<std::chrono::system_clock> timestamp;
        int resourceUsage;

        // Constructor
        ResourceTransformation(bool isIncrease, std::chrono::time_point<std::chrono::system_clock> timestamp, int resourceUsage);
    };
}

#endif //CONTROLLER_RESOURCETRANSFORMATION_H
