//
// Created by jamiec on 9/23/22.
//

#include "WorkItem.h"

#include <utility>

namespace model {
    int WorkItem::internal_item_id_counter = 0;

    enums::request_type WorkItem::getRequestType() const {
        return requestType;
    }

    void WorkItem::setRequestType(enums::request_type requestType) {
        WorkItem::requestType = requestType;
    }

    WorkItem::WorkItem(): internal_id(
            internal_item_id_counter) { internal_item_id_counter++; }

    WorkItem::WorkItem(enums::request_type requestType) : requestType(
            requestType), internal_id(
            internal_item_id_counter) { internal_item_id_counter++; }

    std::shared_ptr<std::vector<std::string>> &WorkItem::getHostList() {
        return host_list;
    }

    WorkItem::WorkItem(std::shared_ptr<std::vector<std::string>> hostList, enums::request_type requestType)
            : host_list(std::move(hostList)), requestType(requestType) { internal_item_id_counter++; }


} // model