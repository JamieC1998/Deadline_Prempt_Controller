//
// Created by jamiec on 9/27/22.
//

#include "Network.h"
#include "../Bucket/Bucket.h"

namespace model {
    std::map<std::string, std::shared_ptr<ComputationDevice>> &Network::getDevices() {
        return devices;
    }

    std::vector<std::shared_ptr<NetCommunicationBase>> &Network::getLink() {
        return network_link;
    }

    void Network::addComm(std::shared_ptr<NetCommunicationBase> linkAct) {
        network_link.push_back(linkAct);
    }

    void Network::addComms(std::vector<std::shared_ptr<NetCommunicationBase>> linkActs) {
        for (auto i: linkActs) {
            network_link.push_back(i);
        }
    }

    void Network::sortLink() {
        std::sort(network_link.begin(), network_link.end(),
                  [](const std::shared_ptr<NetCommunicationBase> &a, const std::shared_ptr<NetCommunicationBase> &b) {
                      return a->timeWindow->stop < b->timeWindow->stop;
                  });
    }

    web::json::value Network::convertToJson() {
        web::json::value result;
        std::vector<web::json::value> computation_list;
        for (const auto &[device_id, device]: Network::devices)
            computation_list.push_back(device->convertToJson());

        result["devices"] = web::json::value::array(computation_list);

        std::vector<web::json::value> comm_list;

        for (const auto &link_item: Network::getLink()) {
            if (link_item->type == contiguous)
                comm_list.push_back(std::static_pointer_cast<model::LinkAct>(link_item)->convertToJson());
            else
                comm_list.push_back(std::static_pointer_cast<model::Bucket>(link_item)->convertToJson());
        }

        result["link"] = web::json::value::array(comm_list);
        return result;
    }

    Network::Network() {
    }
} // model