//
// Created by jamiec on 9/23/22.
//

#ifndef CONTROLLER_WORKQUEUEMANAGER_H
#define CONTROLLER_WORKQUEUEMANAGER_H

#include <queue>
#include <cpprest/http_msg.h>
#include "../../model/enums/RequestTypeEnum.h"
#include "../../model/data_models/WorkItems/BaseWorkItem/WorkItem.h"
#include "../../model/data_models/Network/Network.h"

namespace services {

    class WorkQueueManager {
    public:
        void add_task(model::WorkItem* item);

        static void dag_disruption_call(model::WorkItem* item);

        static void low_comp_allocation_call(model::WorkItem* item);

        static void high_comp_allocation_call(model::WorkItem* item);

        static void deadline_premp_call(model::WorkItem* item);

        static void halt_call();

        [[noreturn]] void main_loop();
    private:
        std::mutex work_queue_lock;
        static std::mutex network_lock;

        static std::shared_ptr<model::Network> network;
        static std::atomic<int> thread_counter;
        std::vector<model::WorkItem*> current_task;
        //Maybe needs to be a dequeue
        std::vector<model::WorkItem*> work_queue;
    };

} // services

#endif //CONTROLLER_WORKQUEUEMANAGER_H
