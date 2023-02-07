//
// Created by jamiec on 9/27/22.
//

#ifndef CONTROLLER_DAGDISRUPTION_H
#define CONTROLLER_DAGDISRUPTION_H

#include "../BaseWorkItem/WorkItem.h"
#include "chrono"

namespace model {

    class DAGDisruption: public WorkItem {
    public:
        DAGDisruption(const std::shared_ptr<std::vector<std::string>> &hostList, enums::request_type requestType,
                      const std::string &partitionedDnnId, const std::string &convidx, int partitionId,
                      const std::chrono::time_point<std::chrono::system_clock> &finishTime);

        explicit DAGDisruption(std::string partitionedDnnId);

        std::string getPartitionedDnnId() const;

        void setPartitionedDnnId(std::string partitionedDnnId);

        std::string getConvidx() const;

        void setConvidx(std::string blockId);

        int getPartitionId() const;

        void setPartitionId(int partitionId);

        const std::chrono::time_point<std::chrono::system_clock> &getFinishTime() const;

        void setFinishTime(const std::chrono::time_point<std::chrono::system_clock> &finishTime);

        const std::chrono::time_point<std::chrono::system_clock> &getEstimatedFinish() const;

        void setEstimatedFinish(const std::chrono::time_point<std::chrono::system_clock> &estimatedFinish);

    private:
        std::string partitioned_dnn_id;
        std::string convidx;
        int partition_id;
        std::chrono::time_point<std::chrono::system_clock> finish_time;
        std::chrono::time_point<std::chrono::system_clock> estimated_finish;
    };

} // model

#endif //CONTROLLER_DAGDISRUPTION_H
