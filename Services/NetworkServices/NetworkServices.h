//
// Created by jamiec on 10/4/22.
//

#ifndef CONTROLLER_NETWORKSERVICES_H
#define CONTROLLER_NETWORKSERVICES_H

#include <utility>
#include <memory>
#include <chrono>
#include <vector>
#include "../../model/data_models/LinkAct/LinkAct.h"

namespace services {

    std::shared_ptr<std::pair<std::chrono::time_point<std::chrono::system_clock>, std::chrono::time_point<std::chrono::system_clock>>>
    findLinkSlot(std::chrono::time_point<std::chrono::system_clock> baseStart, float bW_bytes, uint64_t dataSize, std::vector<std::shared_ptr<model::LinkAct>> netLink);


} // services

#endif //CONTROLLER_NETWORKSERVICES_H
