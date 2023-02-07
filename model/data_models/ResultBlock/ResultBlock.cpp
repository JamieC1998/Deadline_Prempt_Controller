//
// Created by Jamie Cotter on 04/02/2023.
//

#include "ResultBlock.h"

namespace model {
    ResultBlock::ResultBlock() {}

    ResultBlock::ResultBlock(int n, int m) : N(n), M(m) {}

    int ResultBlock::getN() const {
        return N;
    }

    void ResultBlock::setN(int n) {
        N = n;
    }

    int ResultBlock::getM() const {
        return M;
    }

    void ResultBlock::setM(int m) {
        M = m;
    }

    const std::string &ResultBlock::getAssemblyHost() const {
        return assembly_host;
    }

    void ResultBlock::setAssemblyHost(const std::string &assemblyHost) {
        assembly_host = assemblyHost;
    }

    bool ResultBlock::isCompleted() const {
        return completed;
    }

    void ResultBlock::setCompleted(bool completed) {
        ResultBlock::completed = completed;
    }

    const std::chrono::time_point<std::chrono::system_clock> &ResultBlock::getAssemblyFinTime() const {
        return assembly_fin_time;
    }

    void ResultBlock::setAssemblyFinTime(const std::chrono::time_point<std::chrono::system_clock> &assemblyFinTime) {
        assembly_fin_time = assemblyFinTime;
    }

    int ResultBlock::getAssemblyHostId() const {
        return assembly_host_id;
    }

    void ResultBlock::setAssemblyHostId(int assemblyHostId) {
        assembly_host_id = assemblyHostId;
    }

    const std::chrono::time_point<std::chrono::system_clock> &ResultBlock::getStateUpdateFinTime() const {
        return state_update_fin_time;
    }

    void
    ResultBlock::setStateUpdateFinTime(const std::chrono::time_point<std::chrono::system_clock> &stateUpdateFinTime) {
        state_update_fin_time = stateUpdateFinTime;
    }

    const std::chrono::time_point<std::chrono::system_clock> &ResultBlock::getAssemblyStartTime() const {
        return assembly_start_time;
    }

    void
    ResultBlock::setAssemblyStartTime(const std::chrono::time_point<std::chrono::system_clock> &assemblyStartTime) {
        assembly_start_time = assemblyStartTime;
    }

    web::json::value ResultBlock::convertToJson() {
        web::json::value obj;

        obj[U("N")] = web::json::value::number(ResultBlock::getN());
        obj[U("M")] = web::json::value::number(ResultBlock::getM());
        obj[U("assembly_host_id")] = web::json::value::number(ResultBlock::getAssemblyHostId());
        obj[U("assembly_host")] = web::json::value::string(ResultBlock::getAssemblyHost());
        obj[U("completed")] = web::json::value::boolean(ResultBlock::isCompleted());

        obj[U("assembly_fin_time")] = web::json::value::number(
                std::chrono::duration_cast<std::chrono::milliseconds>(
                        ResultBlock::getAssemblyFinTime().time_since_epoch()).count());
        obj[U("assembly_start_time")] = web::json::value::number(
                std::chrono::duration_cast<std::chrono::milliseconds>(
                        ResultBlock::getAssemblyStartTime().time_since_epoch()).count());
        obj[U("state_update_fin_time")] = web::json::value::number(
                std::chrono::duration_cast<std::chrono::milliseconds>(
                        ResultBlock::getStateUpdateFinTime().time_since_epoch()).count());

        web::json::value partitionedTasksArray = web::json::value::array();
        for (const auto &task: ResultBlock::partitioned_tasks) {
            web::json::value taskObj;
            taskObj[U("id")] = web::json::value::number(task.first);
            taskObj[U("task")] = web::json::value(task.second->convertToJson());
            partitionedTasksArray[partitionedTasksArray.size()] = taskObj;
        }
        obj[U("partitioned_tasks")] = partitionedTasksArray;

        web::json::value assemblyUploadWindowsArray = web::json::value::array();
        for (const auto &window: ResultBlock::assembly_upload_windows) {
            web::json::value windowObj;
            windowObj[U("id")] = web::json::value::number(window.first);
            windowObj[U("window")] = web::json::value(window.second->convertToJson());
            assemblyUploadWindowsArray[assemblyUploadWindowsArray.size()] = windowObj;
        }
        obj[U("assembly_upload_windows")] = assemblyUploadWindowsArray;

        obj[U("state_update")] = web::json::value(ResultBlock::state_update->convertToJson());

        return obj;
    }
} // model