//
// Created by Jamie Cotter on 30/07/2025.
//

#ifndef CONTROLLER_WORKQUEUEFUNCTIONS_H
#define CONTROLLER_WORKQUEUEFUNCTIONS_H

#include "../../model/data_models/WorkItems/BaseWorkItem/WorkItem.h"
#include "../WorkQueueManager/WorkQueueManager.h"

namespace services {
    void state_update_call(std::shared_ptr<model::WorkItem> workItem, WorkQueueManager *queueManager, bool isLight);

    void
    low_comp_allocation_call(std::shared_ptr<model::WorkItem> workItem, WorkQueueManager *queueManager, bool isLight);

    void
    high_comp_allocation_call(std::shared_ptr<model::WorkItem> workItem, WorkQueueManager *queueManager, bool isLight);

    void halt_call(std::shared_ptr<model::WorkItem> workItem, WorkQueueManager *queueManager, bool isLight);

	void regenerate_res_data_structure(std::shared_ptr<model::WorkItem> workItem, WorkQueueManager *queueManager);

	void
	update_network_disc(std::shared_ptr<model::WorkItem> workItem, WorkQueueManager *queueManager);

	void
	update_bw_vals(std::shared_ptr<model::WorkItem> workItem, WorkQueueManager *queueManager);

	bool calculate_load_switch(std::chrono::time_point<std::chrono::system_clock> current_time, std::chrono::time_point<std::chrono::system_clock> deadline);

} // services

#endif //CONTROLLER_WORKQUEUEFUNCTIONS_H
