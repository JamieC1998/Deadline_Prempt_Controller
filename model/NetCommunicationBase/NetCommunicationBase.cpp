//
// Created by Jamie Cotter on 23/07/2025.
//

#include "NetCommunicationBase.h"

namespace model {
    int NetCommunicationBase::link_activity_counter = 0;

    NetCommunicationBase::NetCommunicationBase(const std::shared_ptr<TimeWindow> &startFinTime, CommType c_type)
            : timeWindow(
            startFinTime), type(c_type), link_activity_id(link_activity_counter) { link_activity_counter++; }

    NetCommunicationBase::NetCommunicationBase(CommType c_type) : type(c_type), link_activity_id(
            link_activity_counter) { link_activity_counter++; }
} // model