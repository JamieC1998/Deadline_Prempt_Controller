//
// Created by jamiec on 9/27/22.
//

#include "DAGDisruption.h"

namespace model {
    DAGDisruption::DAGDisruption(enums::request_type requestType,
                                 const std::shared_ptr<std::vector<std::string>> &hostList, int partitionedDnnId,
                                 int blockId, int partitionId, int layerId) : WorkItem(requestType, hostList),
                                                                              partitioned_dnn_id(partitionedDnnId),
                                                                              block_id(blockId),
                                                                              partition_id(partitionId),
                                                                              layer_id(layerId) {}

    DAGDisruption::DAGDisruption(int partitionedDnnId) : partitioned_dnn_id(partitionedDnnId) {}

    int DAGDisruption::getPartitionedDnnId() const {
        return partitioned_dnn_id;
    }

    void DAGDisruption::setPartitionedDnnId(int partitionedDnnId) {
        partitioned_dnn_id = partitionedDnnId;
    }

    int DAGDisruption::getBlockId() const {
        return block_id;
    }

    void DAGDisruption::setBlockId(int blockId) {
        block_id = blockId;
    }

    int DAGDisruption::getPartitionId() const {
        return partition_id;
    }

    void DAGDisruption::setPartitionId(int partitionId) {
        partition_id = partitionId;
    }

    int DAGDisruption::getLayerId() const {
        return layer_id;
    }

    void DAGDisruption::setLayerId(int layerId) {
        layer_id = layerId;
    }

    const std::chrono::time_point<std::chrono::high_resolution_clock> &DAGDisruption::getFinishTime() const {
        return finish_time;
    }

    void DAGDisruption::setFinishTime(const std::chrono::time_point<std::chrono::high_resolution_clock> &finishTime) {
        finish_time = finishTime;
    }

    const std::chrono::time_point<std::chrono::high_resolution_clock> &DAGDisruption::getEstimatedFinish() const {
        return estimated_finish;
    }

    void DAGDisruption::setEstimatedFinish(
            const std::chrono::time_point<std::chrono::high_resolution_clock> &estimatedFinish) {
        estimated_finish = estimatedFinish;
    }
} // model