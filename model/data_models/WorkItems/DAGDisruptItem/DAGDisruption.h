//
// Created by jamiec on 9/27/22.
//

#ifndef CONTROLLER_DAGDISRUPTION_H
#define CONTROLLER_DAGDISRUPTION_H

#include "../BaseWorkItem/WorkItem.h"
#include "chrono"

namespace model {

    class DAGDisruption: WorkItem {
    public:
        DAGDisruption(enums::request_type requestType, const std::shared_ptr<std::vector<std::string>> &hostList,
                      int partitionedDnnId, int blockId, int partitionId, int layerId);

        explicit DAGDisruption(int partitionedDnnId);

        int getPartitionedDnnId() const;

        void setPartitionedDnnId(int partitionedDnnId);

        int getBlockId() const;

        void setBlockId(int blockId);

        int getPartitionId() const;

        void setPartitionId(int partitionId);

        int getLayerId() const;

        void setLayerId(int layerId);

        const std::chrono::time_point<std::chrono::high_resolution_clock> &getFinishTime() const;

        void setFinishTime(const std::chrono::time_point<std::chrono::high_resolution_clock> &finishTime);

        const std::chrono::time_point<std::chrono::high_resolution_clock> &getEstimatedFinish() const;

        void setEstimatedFinish(const std::chrono::time_point<std::chrono::high_resolution_clock> &estimatedFinish);

    private:
        int partitioned_dnn_id;
        int block_id;
        int partition_id;
        int layer_id;
        std::chrono::time_point<std::chrono::high_resolution_clock> finish_time;
        std::chrono::time_point<std::chrono::high_resolution_clock> estimated_finish;
    };

} // model

#endif //CONTROLLER_DAGDISRUPTION_H
