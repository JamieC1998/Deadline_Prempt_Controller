//
// Created by Jamie Cotter on 24/06/2025.
//

#include "IsProfilingSingleton.h"

namespace model {
	// Initialize static members outside the class definition
	std::mutex IsProfilingSingleton::mtx;
	IsProfilingSingleton *IsProfilingSingleton::instance = nullptr;

    IsProfilingSingleton *IsProfilingSingleton::getInstance() {
        // Acquire lock before checking instance
        std::lock_guard<std::mutex> lock(mtx); // automatically releases lock
        if (instance == nullptr) {
            instance = new IsProfilingSingleton(true); // Create the instance only once
        }

        return instance;
    }

    bool IsProfilingSingleton::setIsProfiling(bool profile_flag) {
        // Acquire lock before checking instance
        std::lock_guard<std::mutex> lock(mtx); // automatically releases lock
        if (instance == nullptr) {
            instance = new IsProfilingSingleton(true); // Create the instance only once
        }
        instance->isProfiling = profile_flag;
        return instance->isProfiling;
    }

    bool IsProfilingSingleton::getIsProfiling() {
        // Acquire lock before checking instance
        std::lock_guard<std::mutex> lock(mtx); // automatically releases lock
        if (instance == nullptr) {
            instance = new IsProfilingSingleton(true); // Create the instance only once
        }

        return instance->isProfiling;
    }
} // model