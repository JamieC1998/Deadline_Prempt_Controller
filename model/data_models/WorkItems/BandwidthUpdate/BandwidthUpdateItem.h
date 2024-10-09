//
// Created by Jamie Cotter on 03/10/2024.
//

#ifndef CONTROLLER_BANDWIDTHUPDATEITEM_H
#define CONTROLLER_BANDWIDTHUPDATEITEM_H

#include "../BaseWorkItem/WorkItem.h"

namespace model {

    class BandwidthUpdateItem: public WorkItem {
    public:
        std::vector<double> bits_per_second_vect = {};

        BandwidthUpdateItem(enums::request_type requestType, std::vector<double> bitsPerSecondVect);

    };

} // model

#endif //CONTROLLER_BANDWIDTHUPDATEITEM_H
