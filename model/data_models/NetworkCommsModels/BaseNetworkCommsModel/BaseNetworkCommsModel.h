//
// Created by jamiec on 10/11/22.
//

#ifndef CONTROLLER_BASENETWORKCOMMSMODEL_H
#define CONTROLLER_BASENETWORKCOMMSMODEL_H


#include <vector>
#include <string>
#include <chrono>
#include "../../../enums/NetworkCommsTypes.h"
#include "../../Task/Task.h"

namespace model {
    class BaseNetworkCommsModel {

    public:
        const std::vector<std::string> &getHosts() const;

        enums::network_comms_types getType() const;

        const std::chrono::time_point<std::chrono::system_clock> &getCommTime() const;

        BaseNetworkCommsModel(const std::vector<std::string> &hosts, enums::network_comms_types type,
                              const std::chrono::time_point<std::chrono::system_clock> &commTime);

    private:
        std::vector<std::string> hosts;
        enums::network_comms_types type;
        std::chrono::time_point<std::chrono::system_clock> comm_time;

        Task allocatedTask;

    };
}


#endif //CONTROLLER_BASENETWORKCOMMSMODEL_H
