//
// Created by jamiec on 9/27/22.
//

#ifndef CONTROLLER_DAGDISRUPTION_H
#define CONTROLLER_DAGDISRUPTION_H

#include "../BaseWorkItem/WorkItem.h"

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

    private:
        int partitioned_dnn_id;
        int block_id;
        int partition_id;
        int layer_id;
    };

} // model

#endif //CONTROLLER_DAGDISRUPTION_H
