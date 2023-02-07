//
// Created by jamiec on 10/11/22.
//

#ifndef CONTROLLER_NETWORKQUEUEMANAGER_H
#define CONTROLLER_NETWORKQUEUEMANAGER_H


#include <mutex>
#include <vector>
#include <memory>
#include <cpprest/json.h>
#include "../../model/data_models/NetworkCommsModels/BaseNetworkCommsModel/BaseNetworkCommsModel.h"

namespace services {
    class NetworkQueueManager {
    public:
        explicit NetworkQueueManager(const std::vector<std::string> &hosts);

        std::mutex &getNetworkMutex();

        [[noreturn]] void initNetworkCommLoop();

        void addTask(std::shared_ptr<model::BaseNetworkCommsModel> comm_model);

        const std::vector<std::string> &getHosts() const;

    private:
        std::mutex networkMutex;
        std::vector<std::shared_ptr<model::BaseNetworkCommsModel>> comms;
        std::vector<std::string> hosts;

    };

    static void haltReq(std::shared_ptr<model::BaseNetworkCommsModel> comm_model, NetworkQueueManager* queueManager);
    static void highTaskAllocation(std::shared_ptr<model::BaseNetworkCommsModel> comm_model, NetworkQueueManager* queueManager);
    static void taskUpdate(std::shared_ptr<model::BaseNetworkCommsModel> comm_model, NetworkQueueManager* queueManager);
    static void lowTaskAllocation(std::shared_ptr<model::BaseNetworkCommsModel> comm_model, NetworkQueueManager* queueManager);
    static void highTaskReallocation(std::shared_ptr<model::BaseNetworkCommsModel> comm_model, NetworkQueueManager* queueManager);

}
#endif //CONTROLLER_NETWORKQUEUEMANAGER_H
