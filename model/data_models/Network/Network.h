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
#include "../Bucket/Bucket.h"

namespace model {

    class Network {
    public:
        explicit Network();
        std::chrono::time_point<std::chrono::system_clock> last_time_of_reasoning = std::chrono::system_clock::now();
        std::vector<std::shared_ptr<model::Bucket>> network_link;
        std::map<std::string, std::shared_ptr<ComputationDevice>>& getDevices();

        std::map<std::string, std::shared_ptr<ComputationDevice>> devices;
        web::json::value convertToJson();
    };

} // model

#endif //CONTROLLER_NETWORK_H
