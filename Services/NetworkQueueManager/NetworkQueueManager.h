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
#include "../LOG_MANAGER/LogManager.h"

namespace services {
    class NetworkQueueManager {
    public:
        explicit NetworkQueueManager(std::shared_ptr<services::LogManager> ptr);

        std::mutex &getNetworkMutex();

        [[noreturn]] static void initNetworkCommLoop(const std::shared_ptr<services::NetworkQueueManager>& queueManager);

        void addTask(std::shared_ptr<model::BaseNetworkCommsModel> comm_model);

        [[nodiscard]] const std::vector<std::string> &getHosts() const;
        std::vector<std::string> hosts;

        std::shared_ptr<services::LogManager> logManager;
    private:
        std::mutex networkMutex;
        std::vector<std::shared_ptr<model::BaseNetworkCommsModel>> comms;
    };

    static void haltReq(std::shared_ptr<model::BaseNetworkCommsModel> comm_model, std::shared_ptr<services::NetworkQueueManager> queueManager);
    static void highTaskAllocation(const std::shared_ptr<model::BaseNetworkCommsModel>& comm_model, const std::shared_ptr<services::NetworkQueueManager>& queueManager);
    static void lowTaskAllocation(std::shared_ptr<model::BaseNetworkCommsModel> comm_model, std::shared_ptr<services::NetworkQueueManager> queueManager);
    static void initialise_experiment(const std::shared_ptr<services::NetworkQueueManager>& queueManager);
}
#endif //CONTROLLER_NETWORKQUEUEMANAGER_H
