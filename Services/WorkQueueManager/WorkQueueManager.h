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
#include "../../model/data_models/FTP_Lookup/FTP_Lookup.h"
#include "../../model/data_models/CompResult/LowCompResult/LowCompResult.h"

namespace services {

    class WorkQueueManager {
    public:
        WorkQueueManager();

        void add_task(std::shared_ptr<model::WorkItem> item);

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

        double getBytesPerMillisecond();

        void setJitter(double jitter);

        std::map<std::string, std::shared_ptr<model::BaseCompResult>> &getOffTotal();

        std::map<std::string, std::shared_ptr<model::LowCompResult>> &getOffLow();

        static std::map<std::string, std::shared_ptr<model::HighCompResult>> &getOffHigh();

        std::shared_ptr<model::FTP_Lookup> lookup_table;
        uint64_t state_size = 0;

        std::map<std::string, std::shared_ptr<model::BaseCompResult>> off_total;
        std::map<std::string, std::shared_ptr<model::LowCompResult>> off_low;
        static std::map<std::string, std::shared_ptr<model::HighCompResult>> off_high;
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

    static void prune_dnn_call(std::shared_ptr<model::WorkItem> workItem, WorkQueueManager* queueManager);

    static void dag_disruption_call(std::shared_ptr<model::WorkItem> workItem, WorkQueueManager* queueManager);

    static void halt_call(WorkQueueManager* queueManager);

} // services

#endif //CONTROLLER_WORKQUEUEMANAGER_H
