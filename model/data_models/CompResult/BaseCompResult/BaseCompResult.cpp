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

    const std::chrono::time_point<std::chrono::system_clock> &BaseCompResult::getEstimatedStart() const {
        return estimatedStart;
    }

    void BaseCompResult::setEstimatedStart(const std::chrono::time_point<std::chrono::system_clock> &estimatedStart) {
        BaseCompResult::estimatedStart = estimatedStart;
    }

    const std::chrono::time_point<std::chrono::system_clock> &BaseCompResult::getEstimatedFinish() const {
        return estimatedFinish;
    }

    void BaseCompResult::setEstimatedFinish(const std::chrono::time_point<std::chrono::system_clock> &estimatedFinish) {
        BaseCompResult::estimatedFinish = estimatedFinish;
    }

    const std::shared_ptr<LinkAct> &BaseCompResult::getUploadData() const {
        return upload_data;
    }

    void BaseCompResult::setUploadData(const std::shared_ptr<LinkAct> &uploadData) {
        upload_data = uploadData;
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
                                   std::shared_ptr<LinkAct> uploadData, enums::dnn_type dnnType) : dnn_id(
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
                                                                                                   estimatedStart(
                                                                                                           estimatedStart),
                                                                                                   estimatedFinish(
                                                                                                           estimatedFinish),
                                                                                                   upload_data(
                                                                                                           std::move(
                                                                                                                   uploadData)),
                                                                                                   dnnType(dnnType),
                                                                                                   uniqueDnnId(
                                                                                                           uniqueDnnIdCounter) { uniqueDnnIdCounter++; }

    BaseCompResult::BaseCompResult(std::string dnnId, std::string srcHost, int coreAllocation,
                                   const std::chrono::time_point<std::chrono::system_clock> &deadline,
                                   const std::chrono::time_point<std::chrono::system_clock> &estimatedStart,
                                   const std::chrono::time_point<std::chrono::system_clock> &estimatedFinish,
                                   std::shared_ptr<LinkAct> uploadData, enums::dnn_type dnnType) : dnn_id(
            std::move(dnnId)),
                                                                                                   srcHost(
                                                                                                           std::move(
                                                                                                                   srcHost)),
                                                                                                   core_allocation(
                                                                                                           coreAllocation),
                                                                                                   deadline(deadline),
                                                                                                   estimatedStart(
                                                                                                           estimatedStart),
                                                                                                   estimatedFinish(
                                                                                                           estimatedFinish),
                                                                                                   upload_data(
                                                                                                           std::move(
                                                                                                                   uploadData)),
                                                                                                   dnnType(dnnType),
                                                                                                   uniqueDnnId(
                                                                                                           uniqueDnnIdCounter) {
        uniqueDnnIdCounter++;
    }

    BaseCompResult::BaseCompResult(std::string dnnId, std::string srcHost,
                                   const std::chrono::time_point<std::chrono::system_clock> &deadline,
                                   std::shared_ptr<LinkAct> uploadData, enums::dnn_type dnnType) : dnn_id(
            std::move(dnnId)),
                                                                                                   srcHost(std::move(
                                                                                                           srcHost)),
                                                                                                   deadline(deadline),
                                                                                                   upload_data(
                                                                                                           std::move(
                                                                                                                   uploadData)),
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

    const std::shared_ptr<LinkAct> &BaseCompResult::getStateUpdate() const {
        return state_update;
    }

    void BaseCompResult::setStateUpdate(const std::shared_ptr<LinkAct> &stateUpdate) {
        state_update = stateUpdate;
    }

    BaseCompResult::BaseCompResult(std::string dnnId, std::string srcHost,
                                   const std::chrono::time_point<std::chrono::system_clock> &deadline,
                                   enums::dnn_type dnnType) : dnn_id(dnnId), srcHost(srcHost), deadline(deadline),
                                                              dnnType(dnnType), uniqueDnnId(uniqueDnnIdCounter) {
        uniqueDnnIdCounter++;
    }


} // model