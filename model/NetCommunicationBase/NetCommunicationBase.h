//
// Created by Jamie Cotter on 23/07/2025.
//

#ifndef CONTROLLER_NETCOMMUNICATIONBASE_H
#define CONTROLLER_NETCOMMUNICATIONBASE_H

#include <memory>
#include "../data_models/TimeWindow/TimeWindow.h"

namespace model {
    enum CommType { discrete, contiguous };

    class NetCommunicationBase {
        static int link_activity_counter;

    public:
        NetCommunicationBase(const std::shared_ptr<TimeWindow> &startFinTime, CommType c_type);
        NetCommunicationBase(CommType c_type);

    public:
        std::shared_ptr<TimeWindow> timeWindow;
        int link_activity_id = 0;
        CommType type;
    };

} // model

#endif //CONTROLLER_NETCOMMUNICATIONBASE_H
