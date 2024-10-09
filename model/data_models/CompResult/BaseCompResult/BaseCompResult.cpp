//
// Created by Jamie Cotter on 05/02/2023.
//

#include "BaseCompResult.h"

#include <utility>

namespace model {
    int BaseCompResult::uniqueDnnIdCounter = 0;

    BaseCompResult::BaseCompResult(enums::dnn_type dnn_type) : uniqueDnnId(uniqueDnnIdCounter),
                                                               dnnType(dnn_type) { uniqueDnnIdCounter++; }

    int BaseCompResult::getUniqueDnnId() const {
        return uniqueDnnId;
    }

    const std::string &BaseCompResult::getDnnId() const {
        return dnn_id;
    }

    void BaseCompResult::setDnnId(const std::string &dnnId) {
        dnn_id = dnnId;
    }

    const std::string &BaseCompResult::getAllocatedHost() const {
        return allocated_host;
    }

    void BaseCompResult::setAllocatedHost(const std::string &allocatedHost) {
        allocated_host = allocatedHost;
    }

    const std::string &BaseCompResult::getSrcHost() const {
        return srcHost;
    }

    void BaseCompResult::setSrcHost(const std::string &srcHost) {
        BaseCompResult::srcHost = srcHost;
    }

    int BaseCompResult::getCoreAllocation() const {
        return core_allocation;
    }

    void BaseCompResult::setCoreAllocation(int coreAllocation) {
        core_allocation = coreAllocation;
    }

    const std::chrono::time_point<std::chrono::system_clock> &BaseCompResult::getDeadline() const {
        return deadline;
    }

    void BaseCompResult::setDeadline(const std::chrono::time_point<std::chrono::system_clock> &deadline) {
        BaseCompResult::deadline = deadline;
    }

    enums::dnn_type BaseCompResult::getDnnType() const {
        return dnnType;
    }

    void BaseCompResult::setDnnType(enums::dnn_type dnnType) {
        BaseCompResult::dnnType = dnnType;
    }

    BaseCompResult::BaseCompResult(std::string dnnId, std::string allocatedHost,
                                   std::string srcHost, int coreAllocation,
                                   const std::chrono::time_point<std::chrono::system_clock> &deadline,
                                   const std::chrono::time_point<std::chrono::system_clock> &estimatedStart,
                                   const std::chrono::time_point<std::chrono::system_clock> &estimatedFinish,
                                   enums::dnn_type dnnType) : dnn_id(
            std::move(dnnId)),
                                                              allocated_host(
                                                                      std::move(
                                                                              allocatedHost)),
                                                              srcHost(std::move(
                                                                      srcHost)),
                                                              core_allocation(
                                                                      coreAllocation),
                                                              deadline(
                                                                      deadline),
                                                              estimated_start_fin(
                                                                      std::make_shared<TimeWindow>(estimatedStart,
                                                                                                   estimatedFinish)),
                                                              dnnType(dnnType),
                                                              uniqueDnnId(
                                                                      uniqueDnnIdCounter) { uniqueDnnIdCounter++; }

    BaseCompResult::BaseCompResult(std::string dnnId, std::string srcHost, int coreAllocation,
                                   const std::chrono::time_point<std::chrono::system_clock> &deadline,
                                   const std::chrono::time_point<std::chrono::system_clock> &estimatedStart,
                                   const std::chrono::time_point<std::chrono::system_clock> &estimatedFinish,
                                   enums::dnn_type dnnType) : dnn_id(
            std::move(dnnId)),
                                                              srcHost(
                                                                      std::move(
                                                                              srcHost)),
                                                              core_allocation(
                                                                      coreAllocation),
                                                              deadline(deadline),
                                                              estimated_start_fin(
                                                                      std::make_shared<TimeWindow>(estimatedStart,
                                                                                                   estimatedFinish)),
                                                              dnnType(dnnType),
                                                              uniqueDnnId(
                                                                      uniqueDnnIdCounter) {
        uniqueDnnIdCounter++;
    }

    BaseCompResult::BaseCompResult(std::string dnnId, std::string srcHost,
                                   const std::chrono::time_point<std::chrono::system_clock> &deadline,
                                   enums::dnn_type dnnType) : dnn_id(std::move(dnnId)),
                                                              srcHost(std::move(srcHost)),
                                                              deadline(deadline),
                                                              dnnType(dnnType),
                                                              uniqueDnnId(
                                                                      uniqueDnnIdCounter) {
        uniqueDnnIdCounter++;
    }

    const std::chrono::time_point<std::chrono::system_clock> &BaseCompResult::getActualFinish() const {
        return actualFinish;
    }

    void BaseCompResult::setActualFinish(const std::chrono::time_point<std::chrono::system_clock> &actualFinish) {
        BaseCompResult::actualFinish = actualFinish;
    }


} // model