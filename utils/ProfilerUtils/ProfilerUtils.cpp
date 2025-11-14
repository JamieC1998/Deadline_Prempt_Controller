//
// Created by Jamie Cotter on 21/10/2025.
//

#include "ProfilerUtils.h"

namespace utils {

	int numberOfTasksInNetwork(std::shared_ptr<model::Network> network) {
		int tasks = 0;
		for (const auto &[hostname, device]: network->devices) {
			tasks += static_cast<int>(device->DNNS.size());
		}

		return tasks;
	}
} // utils