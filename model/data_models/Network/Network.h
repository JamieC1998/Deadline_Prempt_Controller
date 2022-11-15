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
        explicit Network(const std::map<std::string, std::shared_ptr<ComputationDevice>> &devices);

        std::vector<std::shared_ptr<LinkAct>>& getLink();
        std::map<std::string, std::shared_ptr<ComputationDevice>>& getDevices();
        void addComm(std::shared_ptr<LinkAct> linkAct);
        void addComms(std::vector<std::shared_ptr<LinkAct>> linkActs);
        void sortLink();

    private:
        std::map<std::string, std::shared_ptr<ComputationDevice>> devices;
        std::vector<std::shared_ptr<LinkAct>> network_link;
    };

} // model

#endif //CONTROLLER_NETWORK_H
