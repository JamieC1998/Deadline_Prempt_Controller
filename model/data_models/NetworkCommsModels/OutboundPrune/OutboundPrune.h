//
// Created by Jamie Cotter on 24/02/2023.
//

#ifndef CONTROLLER_OUTBOUNDPRUNE_H
#define CONTROLLER_OUTBOUNDPRUNE_H

#include "../BaseNetworkCommsModel/BaseNetworkCommsModel.h"

namespace model {

    class OutboundPrune: public BaseNetworkCommsModel {
    public:
        OutboundPrune(enums::network_comms_types type,
                      const std::chrono::time_point<std::chrono::system_clock> &commTime, const std::string &dnnId);

        const std::string &getDnnId() const;

    private:
        std::string dnn_id;
    };

} // model

#endif //CONTROLLER_OUTBOUNDPRUNE_H
