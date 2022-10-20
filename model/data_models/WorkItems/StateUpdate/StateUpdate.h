//
// Created by jamiec on 10/4/22.
//

#ifndef CONTROLLER_STATEUPDATE_H
#define CONTROLLER_STATEUPDATE_H

#include <chrono>
#include "../BaseWorkItem/WorkItem.h"

namespace model {

    class StateUpdate : WorkItem {
    public:
        StateUpdate(enums::request_type requestType,
                    const std::chrono::time_point<std::chrono::system_clock> &timestamp,
                    const std::chrono::time_point<std::chrono::system_clock> &finishTime, int groupBlockId,
                    int blockParentId, int partitionModelId, int allocatedDeviceId, int dnnId);

        const std::chrono::time_point<std::chrono::system_clock> &getTimestamp() const;

        const std::chrono::time_point<std::chrono::system_clock> &getFinishTime() const;

        int getGroupBlockId() const;

        int getBlockParentId() const;

        int getPartitionModelId() const;

        int getAllocatedDeviceId() const;

        int getDnnId() const;

    private:
        std::chrono::time_point<std::chrono::system_clock> timestamp;
        std::chrono::time_point<std::chrono::system_clock> finish_time;
        int group_block_id;
        int block_parent_id;
        int partition_model_id;
        int allocated_device_id;
        int dnn_id;
    };

} // model

#endif //CONTROLLER_STATEUPDATE_H
