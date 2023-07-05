//
// Created by jamiec on 10/11/22.
//

#ifndef CONTROLLER_BASENETWORKCOMMSMODEL_H
#define CONTROLLER_BASENETWORKCOMMSMODEL_H


#include <vector>
#include <string>
#include <chrono>
#include "../../../enums/NetworkCommsTypes.h"

namespace model {
    class BaseNetworkCommsModel {

    public:
        BaseNetworkCommsModel(enums::network_comms_types type,
                              const std::chrono::time_point<std::chrono::system_clock> &commTime);

        enums::network_comms_types getType() const;

        const std::chrono::time_point<std::chrono::system_clock> &getCommTime() const;

    private:
        enums::network_comms_types type;
        std::chrono::time_point<std::chrono::system_clock> comm_time;

    };
}


#endif //CONTROLLER_BASENETWORKCOMMSMODEL_H
