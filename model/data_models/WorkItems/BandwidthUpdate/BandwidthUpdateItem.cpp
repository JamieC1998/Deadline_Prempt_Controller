//
// Created by Jamie Cotter on 03/10/2024.
//

#include "BandwidthUpdateItem.h"

#include <utility>

namespace model {

    BandwidthUpdateItem::BandwidthUpdateItem(enums::request_type requestType, std::vector<double> bitsPerSecondVect)
            : bits_per_second_vect(std::move(
            bitsPerSecondVect)), WorkItem(requestType) {

    }
} // model