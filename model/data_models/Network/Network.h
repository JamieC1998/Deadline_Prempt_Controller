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
#include "../TimeSourceSingleton/TimeSourceSingleton.h"

namespace model {

    class Network {
    public:
        explicit Network();
        std::vector<std::shared_ptr<NetCommunicationBase>> network_link;
		std::vector<std::shared_ptr<model::Bucket>> disc_network_link;
		std::chrono::time_point<std::chrono::system_clock> last_time_of_reasoning = model::TimeSourceSingleton::getTime();

        std::vector<std::shared_ptr<NetCommunicationBase>>& getLink();
        std::map<std::string, std::shared_ptr<ComputationDevice>>& getDevices();
        void addComm(std::shared_ptr<NetCommunicationBase> linkAct);
        void addComms(std::vector<std::shared_ptr<NetCommunicationBase>> linkActs);
        void sortLink();
        std::map<std::string, std::shared_ptr<ComputationDevice>> devices;
        web::json::value convertToJson();
    };

} // model

#endif //CONTROLLER_NETWORK_H
