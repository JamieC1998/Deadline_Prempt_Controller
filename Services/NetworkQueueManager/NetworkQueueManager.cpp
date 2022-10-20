//
// Created by jamiec on 10/11/22.
//

#include "NetworkQueueManager.h"

const std::mutex &NetworkQueueManager::getNetworkMutex() const {
    return networkMutex;
}

[[noreturn]] void NetworkQueueManager::initNetworkCommLoop(){
    std::unique_lock<std::mutex> lock(networkMutex, std::defer_lock);
    while(true){
        lock.lock();

        if(comms.front()->getCommTime() == std::chrono::system_clock::now()){
            model::BaseNetworkCommsModel tmp_comm = *comms.front();
            comms.erase(comms.begin());
            lock.unlock();
            switch (tmp_comm.getType()) {
                case enums::network_comms_types::halt_req:
                    haltReq(tmp_comm);
                    break;
                case enums::network_comms_types::task_mapping:
                    taskMapping(tmp_comm);
                    break;
                case enums::network_comms_types::task_update:
                    taskUpdate(tmp_comm);
                    break;
            }
        }

        lock.unlock();
    }
}

void NetworkQueueManager::taskUpdate(model::BaseNetworkCommsModel comm_model) {

}

void NetworkQueueManager::taskMapping(model::BaseNetworkCommsModel comm_model) {

}

void NetworkQueueManager::haltReq(model::BaseNetworkCommsModel comm_model) {

}


