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
#include "../../model/data_models/BaseResult/BaseResult.h"

namespace services {

    class WorkQueueManager {
    public:
        static void add_task(model::WorkItem* item);

        static void dag_disruption_call(model::WorkItem* item);

        static void low_comp_allocation_call(model::WorkItem* item);

        static void high_comp_allocation_call(model::WorkItem* item);

        static void deadline_premp_call(model::WorkItem* item);

        static void halt_call();

        [[noreturn]] void main_loop();
    private:
        static std::mutex work_queue_lock;
        static std::mutex network_lock;
        static std::mutex offloaded_lock;



        static std::shared_ptr<model::Network> network;
        static std::atomic<int> thread_counter;
        std::vector<model::WorkItem*> current_task;
        //Maybe needs to be a dequeue
        static std::vector<model::WorkItem*> work_queue;
        std::map<std::string, std::shared_ptr<model::BaseResult>> off_low;
        std::map<std::string, std::shared_ptr<model::BaseResult>> off_high;
    };

} // services

#endif //CONTROLLER_WORKQUEUEMANAGER_H
