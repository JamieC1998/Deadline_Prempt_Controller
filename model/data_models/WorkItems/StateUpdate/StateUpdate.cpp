//
// Created by jamiec on 10/4/22.
//

#include "StateUpdate.h"

namespace model {

    StateUpdate::StateUpdate(enums::request_type requestType,
                             const std::chrono::time_point<std::chrono::system_clock> &timestamp,
                             const std::chrono::time_point<std::chrono::system_clock> &finishTime, int groupBlockId,
                             int blockParentId, int partitionModelId, int allocatedDeviceId, int dnnId) : WorkItem(
            requestType), timestamp(timestamp), finish_time(finishTime), group_block_id(groupBlockId), block_parent_id(
            blockParentId), partition_model_id(partitionModelId), allocated_device_id(allocatedDeviceId),
                                                                                                          dnn_id(dnnId) {}

    const std::chrono::time_point<std::chrono::system_clock> &StateUpdate::getTimestamp() const {
        return timestamp;
    }

    const std::chrono::time_point<std::chrono::system_clock> &StateUpdate::getFinishTime() const {
        return finish_time;
    }

    int StateUpdate::getGroupBlockId() const {
        return group_block_id;
    }

    int StateUpdate::getBlockParentId() const {
        return block_parent_id;
    }

    int StateUpdate::getPartitionModelId() const {
        return partition_model_id;
    }

    int StateUpdate::getAllocatedDeviceId() const {
        return allocated_device_id;
    }

    int StateUpdate::getDnnId() const {
        return dnn_id;
    }
} // model