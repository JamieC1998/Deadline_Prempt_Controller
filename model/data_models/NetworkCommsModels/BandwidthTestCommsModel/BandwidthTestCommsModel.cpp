//
// Created by Jamie Cotter on 07/10/2024.
//

#include "BandwidthTestCommsModel.h"

#include <utility>

namespace model {
    BandwidthTestCommsModel::BandwidthTestCommsModel(enums::network_comms_types type,
                                                     const std::chrono::time_point<std::chrono::system_clock> &commTime,
                                                     std::string chosenHost) : BaseNetworkCommsModel(type,
                                                                                                            commTime),
                                                                                      chosen_host(std::move(chosenHost)) {}
} // model