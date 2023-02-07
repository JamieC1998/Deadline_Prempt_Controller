//
// Created by jamiec on 9/26/22.
//

#include "HighProcessingItem.h"

#include <utility>

namespace model {
    std::chrono::time_point<std::chrono::system_clock> &
    HighProcessingItem::getDeadline() {
        return deadline;
    }


    HighProcessingItem::HighProcessingItem(std::shared_ptr<std::vector<std::string>> &hostList,
                                           enums::request_type requestType,
                                           std::chrono::time_point<std::chrono::system_clock> deadline,
                                           std::string dnnId) : WorkItem(hostList, requestType),
                                                                       deadline(std::move(deadline)), dnn_id(std::move(dnnId)) {}

    HighProcessingItem::HighProcessingItem(enums::request_type requestType,
                                           std::shared_ptr<std::vector<std::string>> &hostList) : WorkItem(hostList, requestType) {

    }

    bool HighProcessingItem::isReallocation() const {
        return reallocation;
    }

    void HighProcessingItem::setReallocation(bool reallocation) {
        HighProcessingItem::reallocation = reallocation;
    }

    const std::shared_ptr<model::HighCompResult> &HighProcessingItem::getBaseResult() const {
        return baseResult;
    }

    void HighProcessingItem::setBaseResult(const std::shared_ptr<model::HighCompResult> &baseResult) {
        HighProcessingItem::baseResult = baseResult;
    }

    const std::string &HighProcessingItem::getDnnId() const {
        return dnn_id;
    }

    void HighProcessingItem::setDnnId(const std::string &dnnId) {
        dnn_id = dnnId;
    }


} // model