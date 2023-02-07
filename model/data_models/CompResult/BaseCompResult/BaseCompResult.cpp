//
// Created by Jamie Cotter on 05/02/2023.
//

#include "BaseCompResult.h"

namespace model {
    BaseCompResult::BaseCompResult(const std::string &dnnId, enums::dnn_type dnnType) : dnn_id(dnnId),
                                                                                        dnnType(dnnType) {}

    BaseCompResult::BaseCompResult() {}

    const std::string &BaseCompResult::getDnnId() const {
        return dnn_id;
    }

    void BaseCompResult::setDnnId(const std::string &dnnId) {
        dnn_id = dnnId;
    }

    enums::dnn_type BaseCompResult::getDnnType() const {
        return dnnType;
    }

    void BaseCompResult::setDnnType(enums::dnn_type dnnType) {
        BaseCompResult::dnnType = dnnType;
    }
} // model