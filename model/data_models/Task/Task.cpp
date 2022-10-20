//
// Created by jamiec on 9/27/22.
//

#include "Task.h"

namespace model {
    Task::Task(int dnnId, enums::request_type requestType, int groupBlockId, int blockParentId, int partitionModelId,
               int allocatedDeviceId, const std::shared_ptr<TileRegion> &inMap,
               const std::shared_ptr<TileRegion> &outMap, int fusedTaskCount, const std::vector<int> &originalLayerIds,
               float maxRamReq, float maxStorageReq, std::chrono::time_point<std::chrono::system_clock> estimatedStart, std::chrono::time_point<std::chrono::system_clock> estimatedFinish,
               const std::string &allocatedHost, const std::shared_ptr<LinkAct> &comms) : dnn_id(dnnId),
                                                                                          requestType(requestType),
                                                                                          group_block_id(groupBlockId),
                                                                                          block_parent_id(
                                                                                                  blockParentId),
                                                                                          partition_model_id(
                                                                                                  partitionModelId),
                                                                                          allocated_device_id(
                                                                                                  allocatedDeviceId),
                                                                                          in_map(inMap),
                                                                                          out_map(outMap),
                                                                                          fused_task_count(
                                                                                                  fusedTaskCount),
                                                                                          original_layer_ids(
                                                                                                  originalLayerIds),
                                                                                          MAX_RAM_REQ(maxRamReq),
                                                                                          MAX_STORAGE_REQ(
                                                                                                  maxStorageReq),
                                                                                          estimated_start(
                                                                                                  estimatedStart),
                                                                                          estimated_finish(
                                                                                                  estimatedFinish),
                                                                                          allocated_host(allocatedHost),
                                                                                          comms(comms) {}

    Task::Task() {}

    int Task::getDnnId() const {
        return dnn_id;
    }

    void Task::setDnnId(int dnnId) {
        dnn_id = dnnId;
    }

    enums::request_type Task::getRequestType() const {
        return requestType;
    }

    void Task::setRequestType(enums::request_type requestType) {
        Task::requestType = requestType;
    }

    int Task::getGroupBlockId() const {
        return group_block_id;
    }

    void Task::setGroupBlockId(int groupBlockId) {
        group_block_id = groupBlockId;
    }

    int Task::getBlockParentId() const {
        return block_parent_id;
    }

    void Task::setBlockParentId(int blockParentId) {
        block_parent_id = blockParentId;
    }

    int Task::getPartitionModelId() const {
        return partition_model_id;
    }

    void Task::setPartitionModelId(int partitionModelId) {
        partition_model_id = partitionModelId;
    }

    int Task::getAllocatedDeviceId() const {
        return allocated_device_id;
    }

    void Task::setAllocatedDeviceId(int allocatedDeviceId) {
        allocated_device_id = allocatedDeviceId;
    }

    const std::shared_ptr<TileRegion> &Task::getInMap() const {
        return in_map;
    }

    void Task::setInMap(const std::shared_ptr<TileRegion> &inMap) {
        in_map = inMap;
    }

    const std::shared_ptr<TileRegion> &Task::getOutMap() const {
        return out_map;
    }

    void Task::setOutMap(const std::shared_ptr<TileRegion> &outMap) {
        out_map = outMap;
    }

    int Task::getFusedTaskCount() const {
        return fused_task_count;
    }

    void Task::setFusedTaskCount(int fusedTaskCount) {
        fused_task_count = fusedTaskCount;
    }

    const std::vector<int> &Task::getOriginalLayerIds() const {
        return original_layer_ids;
    }

    void Task::setOriginalLayerIds(const std::vector<int> &originalLayerIds) {
        original_layer_ids = originalLayerIds;
    }

    const std::vector<bool> &Task::getCompleted() const {
        return completed;
    }

    void Task::setCompleted(const std::vector<bool> &completed) {
        Task::completed = completed;
    }

    float Task::getMaxRamReq() const {
        return MAX_RAM_REQ;
    }

    void Task::setMaxRamReq(float maxRamReq) {
        MAX_RAM_REQ = maxRamReq;
    }

    float Task::getMaxStorageReq() const {
        return MAX_STORAGE_REQ;
    }

    void Task::setMaxStorageReq(float maxStorageReq) {
        MAX_STORAGE_REQ = maxStorageReq;
    }

    std::chrono::time_point<std::chrono::system_clock> Task::getEstimatedStart() const {
        return estimated_start;
    }

    void Task::setEstimatedStart(std::chrono::time_point<std::chrono::system_clock> estimatedStart) {
        estimated_start = estimatedStart;
    }

    std::chrono::time_point<std::chrono::system_clock> Task::getEstimatedFinish() const {
        return estimated_finish;
    }

    void Task::setEstimatedFinish(std::chrono::time_point<std::chrono::system_clock> estimatedFinish) {
        estimated_finish = estimatedFinish;
    }

    const std::string &Task::getAllocatedHost() const {
        return allocated_host;
    }

    void Task::setAllocatedHost(const std::string &allocatedHost) {
        allocated_host = allocatedHost;
    }

    const std::shared_ptr<LinkAct> &Task::getComms() const {
        return comms;
    }

    void Task::setComms(const std::shared_ptr<LinkAct> &comms) {
        Task::comms = comms;
    }
} // model