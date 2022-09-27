//
// Created by jamiec on 9/27/22.
//

#include "Network.h"

namespace model {
    Network::Network(const std::map<std::string, std::shared_ptr<ComputationDevice>> &devices) : devices(devices) {}

    std::map<std::string, std::shared_ptr<ComputationDevice>> &Network::getDevices() {
        return devices;
    }

    std::vector<std::shared_ptr<LinkAct>> &Network::getLink() {
        return network_link;
    }
} // model