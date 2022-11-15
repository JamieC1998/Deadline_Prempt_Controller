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
#include "../../model/data_models/WorkItems/BaseWorkItem/WorkItem.h"
#include "../NetworkQueueManager/NetworkQueueManager.h"

namespace services {

    class WorkQueueManager {
    public:
        void add_task(model::WorkItem* item);

        [[noreturn]] void main_loop();

        static void decrementThreadCounter();

        std::mutex work_queue_lock;
        std::mutex network_lock = std::mutex();
        std::mutex offloaded_lock = std::mutex();
        std::shared_ptr<model::Network> network;
        std::shared_ptr<services::NetworkQueueManager> networkQueueManager;

        double getAverageBitsPerSecond() const;

        void setAverageBitsPerSecond(double averageBitsPerSecond);

        double getJitter() const;

        void setJitter(double jitter);

        std::map<std::string, std::shared_ptr<model::BaseResult>> &getOffTotal();

        std::map<std::string, std::shared_ptr<model::BaseResult>> &getOffLow();

        std::map<std::string, std::shared_ptr<model::BaseResult>> &getOffHigh();

    private:
        static std::atomic<int> thread_counter;
        std::vector<model::WorkItem*> current_task;
        //Maybe needs to be a dequeue
        std::vector<model::WorkItem*> work_queue;
        std::map<std::string, std::shared_ptr<model::BaseResult>> off_total;
        std::map<std::string, std::shared_ptr<model::BaseResult>> off_low;
        std::map<std::string, std::shared_ptr<model::BaseResult>> off_high;
        double average_bits_per_second = 0.0;
        double jitter = 0.0;
    };

    static void dag_disruption_call(model::WorkItem* item, WorkQueueManager* queueManager);

    static void low_comp_allocation_call(model::WorkItem* item, WorkQueueManager* queueManager);

    static void high_comp_allocation_call(model::WorkItem* item, WorkQueueManager* queueManager);

    static void deadline_premp_call(model::WorkItem* item, WorkQueueManager* queueManager);

    static void halt_call(WorkQueueManager* queueManager);

} // services

#endif //CONTROLLER_WORKQUEUEMANAGER_H
