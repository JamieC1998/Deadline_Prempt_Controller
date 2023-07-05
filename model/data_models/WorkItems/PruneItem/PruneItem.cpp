//
// Created by Jamie Cotter on 06/02/2023.
//

#include "PruneItem.h"

#include <utility>

namespace model {
    PruneItem::PruneItem(enums::request_type requestType, std::string dnnId)
            : WorkItem(requestType), dnn_id(std::move(dnnId)) {}

    const std::string &PruneItem::getDnnId() const {
        return dnn_id;
    }

    void PruneItem::setDnnId(const std::string &dnnId) {
        dnn_id = dnnId;
    }
} // model