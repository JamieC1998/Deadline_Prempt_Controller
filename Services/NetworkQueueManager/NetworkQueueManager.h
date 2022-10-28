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

class NetworkQueueManager {
public:
    const std::mutex &getNetworkMutex() const;

    [[noreturn]] void initNetworkCommLoop();

    void haltReq(model::BaseNetworkCommsModel comm_model);
    static void taskMapping(model::BaseNetworkCommsModel comm_model);
    static void taskUpdate(model::BaseNetworkCommsModel comm_model);
    void addTask(std::shared_ptr<model::BaseNetworkCommsModel> comm_model);
    static std::shared_ptr<web::json::value> generateTaskJson(model::BaseNetworkCommsModel comm_model, std::string current_block);

private:
    std::mutex networkMutex;
    std::vector<std::shared_ptr<model::BaseNetworkCommsModel>> comms;

};


#endif //CONTROLLER_NETWORKQUEUEMANAGER_H
