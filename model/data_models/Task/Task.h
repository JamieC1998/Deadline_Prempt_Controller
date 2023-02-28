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
        Task(std::string dnnId, enums::dnn_type requestType, std::string convidx, int previousConv, int partitionBlockId,
             int n, int m, const std::chrono::time_point<std::chrono::system_clock> &estimatedStart,
             const std::chrono::time_point<std::chrono::system_clock> &estimatedFinish,
             std::string allocatedHost, std::shared_ptr<LinkAct> inputData, uint64_t taskOutputSizeBytes);

        Task(std::string dnnId, enums::dnn_type requestType,
             const std::chrono::time_point<std::chrono::system_clock> &estimatedStart,
             const std::chrono::time_point<std::chrono::system_clock> &estimatedFinish,
             std::string allocatedHost, std::shared_ptr<LinkAct> inputData);

        Task();

        int getUniqueTaskId() const;

        std::string getDnnId() const;

        enums::dnn_type getRequestType() const;

        const std::string &getConvidx() const;

        int getPreviousConv() const;

        int getPartitionBlockId() const;

        bool isCompleted() const;

        int getN() const;

        int getM() const;

        const std::chrono::time_point<std::chrono::system_clock> &getEstimatedStart() const;

        const std::chrono::time_point<std::chrono::system_clock> &getEstimatedFinish() const;

        const std::string &getAllocatedHost() const;

        const std::shared_ptr<LinkAct> &getInputData() const;

        uint64_t getTaskOutputSizeBytes() const;

        void setDnnId(std::string dnnId);

        void setRequestType(enums::dnn_type requestType);

        void setConvidx(const std::string &convidx);

        void setPreviousConv(int previousConv);

        void setPartitionBlockId(int partitionBlockId);

        void setCompleted(bool completed);

        void setN(int n);

        void setM(int m);

        void setEstimatedStart(const std::chrono::time_point<std::chrono::system_clock> &estimatedStart);

        void setEstimatedFinish(const std::chrono::time_point<std::chrono::system_clock> &estimatedFinish);

        void setAllocatedHost(const std::string &allocatedHost);

        void setInputData(const std::shared_ptr<LinkAct> &inputData);

        void setTaskOutputSizeBytes(uint64_t taskOutputSizeBytes);

        const std::chrono::time_point<std::chrono::system_clock> &getActualFinish() const;

        void setActualFinish(const std::chrono::time_point<std::chrono::system_clock> &actualFinish);

        web::json::value convertToJson();

        std::shared_ptr<LinkAct> input_data;

    private:
        int unique_task_id;
        std::string dnn_id;
        enums::dnn_type requestType;

        std::string convidx;
        int previous_conv;
        int partition_block_id;
        bool completed;

        int N;
        int M;

        std::chrono::time_point<std::chrono::system_clock> estimated_start;
        std::chrono::time_point<std::chrono::system_clock> estimated_finish;
        std::chrono::time_point<std::chrono::system_clock> actual_finish;

        std::string allocated_host;



        uint64_t task_output_size_bytes = 0;
    };

} // model

#endif //CONTROLLER_TASK_H
