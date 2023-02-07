//
// Created by Jamie Cotter on 06/02/2023.
//

#include "PruneItem.h"

#include <utility>

namespace model {
    PruneItem::PruneItem(enums::request_type requestType, std::string dnnId, std::string nextConvBlock)
            : WorkItem(requestType), dnn_id(std::move(dnnId)), next_conv_block(std::move(nextConvBlock)) {}

    const std::string &PruneItem::getDnnId() const {
        return dnn_id;
    }

    void PruneItem::setDnnId(const std::string &dnnId) {
        dnn_id = dnnId;
    }

    const std::string &PruneItem::getNextConvBlock() const {
        return next_conv_block;
    }

    void PruneItem::setNextConvBlock(const std::string &nextConvBlock) {
        next_conv_block = nextConvBlock;
    }
} // model