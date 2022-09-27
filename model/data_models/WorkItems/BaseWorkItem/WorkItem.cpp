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

    WorkItem::WorkItem(enums::request_type requestType, std::shared_ptr<std::vector<std::string>> hostList) : requestType(
            requestType), host_list(std::move(hostList)), internal_id(
            internal_item_id_counter) { internal_item_id_counter++; }

    WorkItem::WorkItem() {}

    const std::vector<std::string> &WorkItem::getHostList() const {
        return host_list;
    }

    void WorkItem::setHostList(const std::vector<std::string> &hostList) {
        host_list = hostList;
    }


} // model