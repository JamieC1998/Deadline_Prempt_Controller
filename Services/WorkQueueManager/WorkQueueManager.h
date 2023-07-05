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
#include "../../model/data_models/CompResult/HighCompResult/HighCompResult.h"
#include "../../model/data_models/WorkItems/BaseWorkItem/WorkItem.h"
#include "../NetworkQueueManager/NetworkQueueManager.h"
#include "../../model/data_models/CompResult/LowCompResult/LowCompResult.h"
#include "../LOG_MANAGER/LogManager.h"

namespace services {

    class WorkQueueManager {
    public:
        explicit WorkQueueManager(std::shared_ptr<LogManager> ptr, std::shared_ptr<NetworkQueueManager> sharedPtr);

        void add_task(std::shared_ptr<model::WorkItem> item);

        [[noreturn]] static void main_loop(WorkQueueManager* queueManager);

        static void decrementThreadCounter();

        std::mutex work_queue_lock;
        std::mutex network_lock = std::mutex();
        std::mutex offloaded_lock = std::mutex();
        std::shared_ptr<model::Network> network;
        std::shared_ptr<services::NetworkQueueManager> networkQueueManager;

        double getAverageBitsPerSecond() const;

        void setAverageBitsPerSecond(double averageBitsPerSecond);

        double getJitter() const;

        double getBytesPerMillisecond();

        void setJitter(double jitter);

        uint64_t state_size = 0;

        std::map<std::string, std::shared_ptr<model::BaseCompResult>> off_total;
        std::map<std::string, std::shared_ptr<model::LowCompResult>> off_low;
        std::map<std::string, std::shared_ptr<model::HighCompResult>> off_high;
        std::shared_ptr<services::LogManager> logManager;
    private:
        static std::atomic<int> thread_counter;
        std::vector<std::shared_ptr<model::WorkItem>> current_task;
        //Maybe needs to be a dequeue
        std::vector<std::shared_ptr<model::WorkItem>> work_queue;
        double average_bits_per_second = 0.0;
        double jitter = 0.0;
    };

    static void state_update_call(std::shared_ptr<model::WorkItem> workItem, WorkQueueManager* queueManager);

    static void low_comp_allocation_call(std::shared_ptr<model::WorkItem> workItem, WorkQueueManager* queueManager);

    static void high_comp_allocation_call(std::shared_ptr<model::WorkItem> workItem, WorkQueueManager* queueManager);

    static void halt_call(std::shared_ptr<model::WorkItem> workItem, WorkQueueManager* queueManager);

} // services

#endif //CONTROLLER_WORKQUEUEMANAGER_H
