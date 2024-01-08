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
#include "../../model/data_models/CompResult/LowCompResult/LowCompResult.h"
#include "../LOG_MANAGER/LogManager.h"
#include "../../model/data_models/WorkItems/ProcessingItem/HighProcessingItem/HighProcessingItem.h"
#include "cpprest/http_client.h"
#include "../../model/data_models/NetworkCommsModels/BaseNetworkCommsModel/BaseNetworkCommsModel.h"

namespace services {

    class WorkQueueManager {
    public:
        explicit WorkQueueManager(std::shared_ptr<LogManager> ptr);

        void add_task(std::shared_ptr<model::WorkItem> item);

        [[noreturn]] static void main_loop(WorkQueueManager* queueManager);

        std::mutex work_queue_lock;
        std::mutex network_lock = std::mutex();
        std::mutex offloaded_lock = std::mutex();
        std::shared_ptr<model::Network> network;

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

        const std::vector<std::shared_ptr<model::HighProcessingItem>> &getWorkStealingQueue() const;

        void setWorkStealingQueue(const std::vector<std::shared_ptr<model::HighProcessingItem>> &workStealingQueue);

    private:
        std::vector<std::shared_ptr<model::WorkItem>> work_queue;
        double average_bits_per_second = 0.0;
        double jitter = 0.0;

        std::vector<std::shared_ptr<model::HighProcessingItem>> work_stealing_queue;
    };

    void state_update_call(std::shared_ptr<model::WorkItem> workItem, WorkQueueManager* queueManager);

    void low_comp_allocation_call(std::shared_ptr<model::WorkItem> workItem, WorkQueueManager* queueManager);

    void high_comp_allocation_call(std::shared_ptr<model::WorkItem> workItem, WorkQueueManager* queueManager);

    void halt_call(std::shared_ptr<model::WorkItem> workItem, WorkQueueManager* queueManager);

    void add_high_comp_work_item(std::shared_ptr<model::WorkItem> item, WorkQueueManager *queueManager);

    void haltReq(std::shared_ptr<model::BaseNetworkCommsModel> comm_model, std::shared_ptr<services::LogManager> logManager);

    void highTaskAllocation(const std::shared_ptr<model::BaseNetworkCommsModel>& comm_model, std::shared_ptr<services::LogManager> logManager);

    void lowTaskAllocation(std::shared_ptr<model::BaseNetworkCommsModel> comm_model, std::shared_ptr<services::LogManager> logManager);

} // services

#endif //CONTROLLER_WORKQUEUEMANAGER_H
