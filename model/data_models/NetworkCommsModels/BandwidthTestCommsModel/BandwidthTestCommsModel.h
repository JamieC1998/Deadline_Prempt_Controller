//
// Created by Jamie Cotter on 07/10/2024.
//

#ifndef CONTROLLER_BANDWIDTHTESTCOMMSMODEL_H
#define CONTROLLER_BANDWIDTHTESTCOMMSMODEL_H

#include "../BaseNetworkCommsModel/BaseNetworkCommsModel.h"

namespace model {

    class BandwidthTestCommsModel: public BaseNetworkCommsModel {
    public:
        std::string chosen_host;

        BandwidthTestCommsModel(enums::network_comms_types type,
                                const std::chrono::time_point<std::chrono::system_clock> &commTime,
                                std::string chosenHost);

    };

} // model

#endif //CONTROLLER_BANDWIDTHTESTCOMMSMODEL_H
