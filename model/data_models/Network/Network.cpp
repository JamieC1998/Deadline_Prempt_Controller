//
// Created by jamiec on 9/27/22.
//

#include "Network.h"

namespace model {
    std::map<std::string, std::shared_ptr<ComputationDevice>> &Network::getDevices() {
        return devices;
    }

    web::json::value Network::convertToJson() {
        web::json::value result;
        std::vector<web::json::value> computation_list;
        computation_list.reserve(Network::devices.size());

        for (const auto &[device_id, device]: Network::devices)
            computation_list.push_back(device->convertToJson());

        result["devices"] = web::json::value::array(computation_list);

        std::vector<web::json::value> comm_list;

        comm_list.reserve(Network::network_link.size());
        for (const auto &link_item: Network::network_link)
            comm_list.push_back(link_item->convertToJson());
        result["link"] = web::json::value::array(comm_list);
        return result;
    }

    Network::Network() {
    }
} // model