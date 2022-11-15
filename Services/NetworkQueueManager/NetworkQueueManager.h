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
        const std::mutex &getNetworkMutex() const;

        [[noreturn]] void initNetworkCommLoop();

        void addTask(std::shared_ptr<model::BaseNetworkCommsModel> comm_model);

    private:
        std::mutex networkMutex;
        std::vector<std::shared_ptr<model::BaseNetworkCommsModel>> comms;

    };

    static std::shared_ptr<web::json::value> generateTaskJson(model::BaseNetworkCommsModel comm_model, std::string current_block);
    static void haltReq(model::BaseNetworkCommsModel comm_model, NetworkQueueManager* queueManager);
    static void taskMapping(model::BaseNetworkCommsModel comm_model, NetworkQueueManager* queueManager);
    static void taskUpdate(model::BaseNetworkCommsModel comm_model, NetworkQueueManager* queueManager);

}
#endif //CONTROLLER_NETWORKQUEUEMANAGER_H
