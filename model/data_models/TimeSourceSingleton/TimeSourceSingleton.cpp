//
// Created by Jamie Cotter on 24/06/2025.
//

#include "TimeSourceSingleton.h"
#include "../IsProfilingSingleton/IsProfilingSingleton.h"
#include "iostream"

namespace model {
	std::mutex TimeSourceSingleton::mtx;
	TimeSourceSingleton *TimeSourceSingleton::instance = nullptr;

    TimeSourceSingleton *TimeSourceSingleton::getInstance() {
        // Acquire lock before checking instance
        std::lock_guard<std::mutex> lock(mtx); // automatically releases lock
        if (TimeSourceSingleton::instance == nullptr) {
			TimeSourceSingleton::instance = new TimeSourceSingleton(); // Create the instance only once
        }

        return TimeSourceSingleton::instance;
    }

    std::chrono::time_point<std::chrono::system_clock> TimeSourceSingleton::getTime() {
        if (IsProfilingSingleton::getIsProfiling()) {
            std::lock_guard<std::mutex> lock(mtx); // automatically releases lock
            if (TimeSourceSingleton::instance == nullptr) {
				TimeSourceSingleton::instance = new TimeSourceSingleton(); // Create the instance only once
            }
			auto temp = TimeSourceSingleton::instance;
            return temp->ct;
        }

        return std::chrono::system_clock::now();
    }

    std::chrono::time_point<std::chrono::system_clock>
    TimeSourceSingleton::setTime(std::chrono::time_point<std::chrono::system_clock> currentTime) {
        if (IsProfilingSingleton::getIsProfiling()) {
            std::lock_guard<std::mutex> lock(mtx); // automatically releases lock
            if (TimeSourceSingleton::instance == nullptr) {
				TimeSourceSingleton::instance = new TimeSourceSingleton(); // Create the instance only once
            }
			TimeSourceSingleton::instance->ct = currentTime;
			auto ti = TimeSourceSingleton::instance->ct;
			auto to = std::chrono::time_point<std::chrono::system_clock>(std::chrono::milliseconds{1750000000000});
			auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(ti.time_since_epoch());
			if(static_cast<uint64_t>(millis.count()) > 1750000000000){
				std::cout << "WOW" << std::endl;
			}

            return TimeSourceSingleton::instance->ct;
        }
		auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(TimeSourceSingleton::instance->ct.time_since_epoch());

		auto to = std::chrono::time_point<std::chrono::system_clock>(std::chrono::milliseconds{1750000000000});
		if(static_cast<uint64_t>(millis.count()) > 1750000000000){
			std::cout << "WOW" << std::endl;
		}
        return TimeSourceSingleton::instance->ct;
    }
} // model