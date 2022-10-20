//
// Created by jamiec on 10/11/22.
//

#ifndef CONTROLLER_NETWORKQUEUEMANAGER_H
#define CONTROLLER_NETWORKQUEUEMANAGER_H


#include <mutex>
#include <vector>
#include <memory>
#include "../../model/data_models/NetworkCommsModels/BaseNetworkCommsModel/BaseNetworkCommsModel.h"

class NetworkQueueManager {
public:
    const std::mutex &getNetworkMutex() const;

    [[noreturn]] void initNetworkCommLoop();

    void haltReq(model::BaseNetworkCommsModel comm_model);
    void taskMapping(model::BaseNetworkCommsModel comm_model);
    void taskUpdate(model::BaseNetworkCommsModel comm_model);

private:
    std::mutex networkMutex;
    std::vector<std::shared_ptr<model::BaseNetworkCommsModel>> comms;

};


#endif //CONTROLLER_NETWORKQUEUEMANAGER_H
