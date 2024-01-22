//
// Created by Jamie Cotter on 28/04/2023.
//

#include "HaltWorkItem.h"

namespace model {


    const std::string &HaltWorkItem::getAllocatedHost() const {
        return allocated_host;
    }

    const std::string &HaltWorkItem::getDnnId() const {
        return dnn_id;
    }

    HaltWorkItem::HaltWorkItem(enums::request_type requestType, std::string alloHost, std::string dnn_id): WorkItem(requestType), allocated_host(alloHost), dnn_id(dnn_id) {

    }
} // model