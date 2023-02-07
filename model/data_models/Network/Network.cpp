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

    void Network::addComm(std::shared_ptr<LinkAct> linkAct) {
        network_link.push_back(linkAct);
    }

    void Network::addComms(std::vector<std::shared_ptr<LinkAct>> linkActs) {
        for(auto i: linkActs){
            network_link.push_back(i);
        }
    }

    void Network::sortLink() {
        std::sort(network_link.begin(), network_link.end(), [](const std::shared_ptr<LinkAct>& a, const std::shared_ptr<LinkAct>& b) {
            return a->getStartFinTime().second < b->getStartFinTime().second;
        });
    }
} // model