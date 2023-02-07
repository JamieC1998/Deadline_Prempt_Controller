//
// Created by jamiec on 9/27/22.
//

#include "DAGDisruption.h"

namespace model {

    DAGDisruption::DAGDisruption(std::string partitionedDnnId) : partitioned_dnn_id(partitionedDnnId) {}

    std::string DAGDisruption::getPartitionedDnnId() const {
        return partitioned_dnn_id;
    }

    void DAGDisruption::setPartitionedDnnId(std::string partitionedDnnId) {
        partitioned_dnn_id = partitionedDnnId;
    }

    std::string DAGDisruption::getConvidx() const {
        return convidx;
    }

    void DAGDisruption::setConvidx(std::string blockId) {
        convidx = blockId;
    }

    int DAGDisruption::getPartitionId() const {
        return partition_id;
    }

    void DAGDisruption::setPartitionId(int partitionId) {
        partition_id = partitionId;
    }

    const std::chrono::time_point<std::chrono::system_clock> &DAGDisruption::getFinishTime() const {
        return finish_time;
    }

    void DAGDisruption::setFinishTime(const std::chrono::time_point<std::chrono::system_clock> &finishTime) {
        finish_time = finishTime;
    }

    const std::chrono::time_point<std::chrono::system_clock> &DAGDisruption::getEstimatedFinish() const {
        return estimated_finish;
    }

    void DAGDisruption::setEstimatedFinish(
            const std::chrono::time_point<std::chrono::system_clock> &estimatedFinish) {
        estimated_finish = estimatedFinish;
    }

    DAGDisruption::DAGDisruption(const std::shared_ptr<std::vector<std::string>> &hostList,
                                 enums::request_type requestType, const std::string &partitionedDnnId,
                                 const std::string &convidx, int partitionId,
                                 const std::chrono::time_point<std::chrono::system_clock> &finishTime) : WorkItem(
            hostList, requestType), partitioned_dnn_id(partitionedDnnId), convidx(convidx), partition_id(partitionId),
                                                                                                         finish_time(
                                                                                                                 finishTime) {}

} // model