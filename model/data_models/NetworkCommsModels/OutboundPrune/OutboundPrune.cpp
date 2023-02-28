//
// Created by Jamie Cotter on 24/02/2023.
//

#include "OutboundPrune.h"

namespace model {
    OutboundPrune::OutboundPrune(enums::network_comms_types type,
                                 const std::chrono::time_point<std::chrono::system_clock> &commTime,
                                 const std::string &dnnId) : BaseNetworkCommsModel(type, commTime), dnn_id(dnnId) {}

    const std::string &OutboundPrune::getDnnId() const {
        return dnn_id;
    }
} // model