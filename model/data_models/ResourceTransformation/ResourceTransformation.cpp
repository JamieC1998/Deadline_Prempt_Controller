//
// Created by Jamie Cotter on 09/09/2024.
//
#include "ResourceTransformation.h"

namespace model {

// Constructor Implementation
    ResourceTransformation::ResourceTransformation(bool isIncrease, std::chrono::time_point<std::chrono::system_clock> timestamp, int resourceUsage)
            : isIncrease(isIncrease), timestamp(timestamp), resourceUsage(resourceUsage) {
        // Constructor body (if needed)
    }
}