//
// Created by jamiec on 9/27/22.
//

#include "Task.h"

#include <utility>

namespace model {
    int Task::taskIdCounter = 0;

    Task::Task(): unique_task_id(taskIdCounter) {taskIdCounter++;}

    Task::Task(std::string dnnId, enums::dnn_type requestType, std::string convidx, int previousConv,
               int partitionBlockId, int n, int m,
               const std::chrono::time_point<std::chrono::system_clock> &estimatedStart,
               const std::chrono::time_point<std::chrono::system_clock> &estimatedFinish,
               std::string allocatedHost, std::shared_ptr<LinkAct> inputData,
               uint64_t taskOutputSizeBytes) : dnn_id(std::move(dnnId)), requestType(requestType), convidx(std::move(convidx)),
                                               previous_conv(previousConv), partition_block_id(partitionBlockId), N(n),
                                               M(m), estimated_start(estimatedStart), estimated_finish(estimatedFinish),
                                               allocated_host(std::move(allocatedHost)), input_data(std::move(inputData)),
                                               task_output_size_bytes(taskOutputSizeBytes), unique_task_id(taskIdCounter) {taskIdCounter++;}

    int Task::getUniqueTaskId() const {
        return unique_task_id;
    }

    std::string Task::getDnnId() const {
        return dnn_id;
    }

    enums::dnn_type Task::getRequestType() const {
        return requestType;
    }

    const std::string &Task::getConvidx() const {
        return convidx;
    }

    int Task::getPreviousConv() const {
        return previous_conv;
    }

    int Task::getPartitionBlockId() const {
        return partition_block_id;
    }

    bool Task::isCompleted() const {
        return completed;
    }

    int Task::getN() const {
        return N;
    }

    int Task::getM() const {
        return M;
    }

    const std::chrono::time_point<std::chrono::system_clock> &Task::getEstimatedStart() const {
        return estimated_start;
    }

    const std::chrono::time_point<std::chrono::system_clock> &Task::getEstimatedFinish() const {
        return estimated_finish;
    }

    const std::string &Task::getAllocatedHost() const {
        return allocated_host;
    }

    const std::shared_ptr<LinkAct> &Task::getInputData() const {
        return input_data;
    }

    uint64_t Task::getTaskOutputSizeBytes() const {
        return task_output_size_bytes;
    }

    void Task::setDnnId(std::string dnnId) {
        dnn_id = dnnId;
    }

    void Task::setRequestType(enums::dnn_type requestType) {
        Task::requestType = requestType;
    }

    void Task::setConvidx(const std::string &convidx) {
        Task::convidx = convidx;
    }

    void Task::setPreviousConv(int previousConv) {
        previous_conv = previousConv;
    }

    void Task::setPartitionBlockId(int partitionBlockId) {
        partition_block_id = partitionBlockId;
    }

    void Task::setCompleted(bool completed) {
        Task::completed = completed;
    }

    void Task::setN(int n) {
        N = n;
    }

    void Task::setM(int m) {
        M = m;
    }

    void Task::setEstimatedStart(const std::chrono::time_point<std::chrono::system_clock> &estimatedStart) {
        estimated_start = estimatedStart;
    }

    void Task::setEstimatedFinish(const std::chrono::time_point<std::chrono::system_clock> &estimatedFinish) {
        estimated_finish = estimatedFinish;
    }

    void Task::setAllocatedHost(const std::string &allocatedHost) {
        allocated_host = allocatedHost;
    }

    void Task::setInputData(const std::shared_ptr<LinkAct> &inputData) {
        input_data = inputData;
    }

    void Task::setTaskOutputSizeBytes(uint64_t taskOutputSizeBytes) {
        task_output_size_bytes = taskOutputSizeBytes;
    }

    const std::chrono::time_point<std::chrono::system_clock> &Task::getActualFinish() const {
        return actual_finish;
    }

    void Task::setActualFinish(const std::chrono::time_point<std::chrono::system_clock> &actualFinish) {
        actual_finish = actualFinish;
    }

    Task::Task(std::string dnnId, enums::dnn_type requestType,
               const std::chrono::time_point<std::chrono::system_clock> &estimatedStart,
               const std::chrono::time_point<std::chrono::system_clock> &estimatedFinish,
               std::string allocatedHost, std::shared_ptr<LinkAct> inputData) : dnn_id(std::move(dnnId)),
                                                                                              requestType(requestType),
                                                                                              estimated_start(
                                                                                                      estimatedStart),
                                                                                              estimated_finish(
                                                                                                      estimatedFinish),
                                                                                              allocated_host(std::move(
                                                                                                      allocatedHost)),
                                                                                              input_data(std::move(inputData)) {}

    web::json::value Task::convertToJson() {
            web::json::value task_json;

            task_json[U("unique_task_id")] = web::json::value::number(Task::getUniqueTaskId());
            task_json[U("dnn_id")] = web::json::value::string(Task::getDnnId());
            task_json[U("convidx")] = web::json::value::string(Task::getConvidx());
            task_json[U("previous_conv")] = web::json::value::number(Task::getPreviousConv());
            task_json[U("partition_block_id")] = web::json::value::number(Task::getPartitionBlockId());
            task_json[U("completed")] = web::json::value::boolean(Task::isCompleted());
            task_json[U("N")] = web::json::value::number(Task::getN());
            task_json[U("M")] = web::json::value::number(Task::getM());

            task_json[U("estimated_start")] = web::json::value::number(std::chrono::duration_cast<std::chrono::milliseconds>(
                    Task::getEstimatedStart().time_since_epoch()).count());
            task_json[U("estimated_finish")] = web::json::value::number(std::chrono::duration_cast<std::chrono::milliseconds>(
                    Task::getEstimatedFinish().time_since_epoch()).count());
            task_json[U("allocated_host")] = web::json::value::string(Task::getAllocatedHost());

            task_json[U("input_data")] = web::json::value(Task::getInputData()->convertToJson());

            task_json[U("task_output_size_bytes")] = web::json::value::number(Task::getTaskOutputSizeBytes());
            task_json[U("actual_finish")] = web::json::value::number(std::chrono::duration_cast<std::chrono::milliseconds>(
                    Task::getActualFinish().time_since_epoch()).count());
        return task_json;
    }


} // model