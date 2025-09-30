//
// Created by Jamie Cotter on 24/06/2025.
//

#ifndef CONTROLLER_TIMESOURCESINGLETON_H
#define CONTROLLER_TIMESOURCESINGLETON_H

#include <mutex>

namespace model {

	class TimeSourceSingleton {

	private:
		TimeSourceSingleton() {}

		TimeSourceSingleton(const TimeSourceSingleton &) = delete;

		TimeSourceSingleton &operator=(const TimeSourceSingleton &) = delete;

		static std::mutex mtx; // Mutex for thread safety
		static TimeSourceSingleton *instance;
		std::chrono::time_point<std::chrono::system_clock> ct = std::chrono::system_clock::now();

	public:
		static TimeSourceSingleton *getInstance();

		static std::chrono::time_point<std::chrono::system_clock> getTime();

		static std::chrono::time_point<std::chrono::system_clock>
		setTime(std::chrono::time_point<std::chrono::system_clock> currentTime);
	};

} // model

#endif //CONTROLLER_TIMESOURCESINGLETON_H
