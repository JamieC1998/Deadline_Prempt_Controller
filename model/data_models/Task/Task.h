//
// Created by jamiec on 9/27/22.
//

#ifndef CONTROLLER_TASK_H
#define CONTROLLER_TASK_H

#include <memory>
#include <vector>
#include <chrono>
#include "../../enums/RequestTypeEnum.h"
#include "../TileRegion/TileRegion.h"
#include "../LinkAct/LinkAct.h"
#include "../../enums/DNN_Type_Enum.h"

namespace model {

    class Task {
        static int taskIdCounter;
    public:
        Task(int dnnId, enums::dnn_type requestType, int groupBlockId, int blockParentId, int partitionModelId,
             int allocatedDeviceId, const std::shared_ptr<TileRegion> &inMap, const std::shared_ptr<TileRegion> &outMap,
             int fusedTaskCount, const std::vector<int> &originalLayerIds, float maxRamReq, float maxStorageReq,
             std::chrono::time_point<std::chrono::high_resolution_clock> estimatedStart, std::chrono::time_point<std::chrono::high_resolution_clock> estimatedFinish, const std::string &allocatedHost,
             const std::shared_ptr<LinkAct> &comms);

        Task();

        int getDnnId() const;

        void setDnnId(int dnnId);

        enums::dnn_type getRequestType() const;

        void setRequestType(enums::dnn_type requestType);

        int getGroupBlockId() const;

        void setGroupBlockId(int groupBlockId);

        int getBlockParentId() const;

        void setBlockParentId(int blockParentId);

        int getPartitionModelId() const;

        void setPartitionModelId(int partitionModelId);

        int getAllocatedDeviceId() const;

        void setAllocatedDeviceId(int allocatedDeviceId);

        const std::shared_ptr<TileRegion> &getInMap() const;

        void setInMap(const std::shared_ptr<TileRegion> &inMap);

        const std::shared_ptr<TileRegion> &getOutMap() const;

        void setOutMap(const std::shared_ptr<TileRegion> &outMap);

        int getFusedTaskCount() const;

        void setFusedTaskCount(int fusedTaskCount);

        const std::vector<int> &getOriginalLayerIds() const;

        void setOriginalLayerIds(const std::vector<int> &originalLayerIds);

        const std::vector<bool> &getCompleted() const;

        void setCompleted(const std::vector<bool> &completed);

        float getMaxRamReq() const;

        void setMaxRamReq(float maxRamReq);

        float getMaxStorageReq() const;

        void setMaxStorageReq(float maxStorageReq);

        std::chrono::time_point<std::chrono::high_resolution_clock> getEstimatedStart() const;

        void setEstimatedStart(std::chrono::time_point<std::chrono::high_resolution_clock> estimatedStart);

        std::chrono::time_point<std::chrono::high_resolution_clock> getEstimatedFinish() const;

        void setEstimatedFinish(std::chrono::time_point<std::chrono::high_resolution_clock> estimatedFinish);

        const std::string &getAllocatedHost() const;

        void setAllocatedHost(const std::string &allocatedHost);

        const std::shared_ptr<LinkAct> &getComms() const;

        void setComms(const std::shared_ptr<LinkAct> &comms);

    private:
        int unique_task_id;
    public:
        int getUniqueTaskId() const;

    private:
        int dnn_id;
        enums::dnn_type requestType;
        int group_block_id;
        int block_parent_id;
        int partition_model_id;
        int allocated_device_id;
        std::shared_ptr<TileRegion> in_map;
        std::shared_ptr<TileRegion> out_map;

        int fused_task_count;
        std::vector<int> original_layer_ids;
        std::vector<bool> completed;

        float MAX_RAM_REQ;
        float MAX_STORAGE_REQ;

        std::chrono::time_point<std::chrono::high_resolution_clock> estimated_start;
        std::chrono::time_point<std::chrono::high_resolution_clock> estimated_finish;

        std::string allocated_host;

        std::shared_ptr<LinkAct> comms;
    };

} // model

#endif //CONTROLLER_TASK_H
