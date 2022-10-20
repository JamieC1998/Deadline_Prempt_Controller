//
// Created by jamiec on 9/27/22.
//

#include "BaseResult.h"

namespace model {
    int BaseResult::dnnIdCounter = 0;

    BaseResult::BaseResult(int sourceDevId, enums::dnn_type dnnType, const std::string &srcHost, std::chrono::time_point<std::chrono::system_clock> deadline,
                           std::chrono::time_point<std::chrono::system_clock> estimatedStart, std::chrono::time_point<std::chrono::system_clock> estimatedFinish) : sourceDevId(sourceDevId), dnnType(dnnType),
                                                                            srcHost(srcHost), deadline(deadline),
                                                                            estimatedStart(estimatedStart),
                                                                            estimatedFinish(estimatedFinish), dnn_id(dnnIdCounter) { dnnIdCounter++; }

    int BaseResult::getSourceDevId() const {
        return sourceDevId;
    }

    void BaseResult::setSourceDevId(int sourceDevId) {
        BaseResult::sourceDevId = sourceDevId;
    }

    enums::dnn_type BaseResult::getDnnType() const {
        return dnnType;
    }

    void BaseResult::setDnnType(enums::dnn_type dnnType) {
        BaseResult::dnnType = dnnType;
    }

    const std::string &BaseResult::getSrcHost() const {
        return srcHost;
    }

    void BaseResult::setSrcHost(const std::string &srcHost) {
        BaseResult::srcHost = srcHost;
    }

    std::chrono::time_point<std::chrono::system_clock> BaseResult::getDeadline() const {
        return deadline;
    }

    void BaseResult::setDeadline(std::chrono::time_point<std::chrono::system_clock> deadline) {
        BaseResult::deadline = deadline;
    }

    std::chrono::time_point<std::chrono::system_clock> BaseResult::getEstimatedStart() const {
        return estimatedStart;
    }

    void BaseResult::setEstimatedStart(std::chrono::time_point<std::chrono::system_clock> estimatedStart) {
        BaseResult::estimatedStart = estimatedStart;
    }

    std::chrono::time_point<std::chrono::system_clock> BaseResult::getEstimatedFinish() const {
        return estimatedFinish;
    }

    void BaseResult::setEstimatedFinish(std::chrono::time_point<std::chrono::system_clock> estimatedFinish) {
        BaseResult::estimatedFinish = estimatedFinish;
    }

    int BaseResult::getDnnId() const {
        return dnn_id;
    }

    void BaseResult::setDnnId(int dnnId) {
        dnn_id = dnnId;
    }
} // model