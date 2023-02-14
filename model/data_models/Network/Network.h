//
// Created by jamiec on 9/27/22.
//

#ifndef CONTROLLER_NETWORK_H
#define CONTROLLER_NETWORK_H

#include <string>
#include <map>
#include <memory>
#include "../ComputationDevice/ComputationDevice.h"
#include "../LinkAct/LinkAct.h"

namespace model {

    class Network {
    public:
        explicit Network();
        std::vector<std::shared_ptr<LinkAct>> network_link;
        std::vector<std::shared_ptr<LinkAct>>& getLink();
        std::map<std::string, std::shared_ptr<ComputationDevice>>& getDevices();
        void addComm(std::shared_ptr<LinkAct> linkAct);
        void addComms(std::vector<std::shared_ptr<LinkAct>> linkActs);
        void sortLink();
        std::map<std::string, std::shared_ptr<ComputationDevice>> devices;
    };

} // model

#endif //CONTROLLER_NETWORK_H
