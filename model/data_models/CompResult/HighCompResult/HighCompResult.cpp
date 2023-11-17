//
// Created by jamiec on 9/27/22.
//

#include "HighCompResult.h"

#include <utility>

namespace model {

    int HighCompResult::getM() const {
        return M;
    }

    void HighCompResult::setM(int m) {
        M = m;
    }

    int HighCompResult::getN() const {
        return N;
    }

    void HighCompResult::setN(int n) {
        N = n;
    }

    const std::shared_ptr<LinkAct> &HighCompResult::getTaskAllocation() const {
        return task_allocation;
    }

    void HighCompResult::setTaskAllocation(const std::shared_ptr<LinkAct> &taskAllocation) {
        task_allocation = taskAllocation;
    }

    uint64_t HighCompResult::getVersion() const {
        return version;
    }

    void HighCompResult::setVersion(uint64_t version) {
        HighCompResult::version = version;
    }

    HighCompResult::HighCompResult(const std::string &dnnId, const std::string &allocatedHost,
                                   const std::string &srcHost, int coreAllocation,
                                   const std::chrono::time_point<std::chrono::system_clock> &deadline,
                                   const std::chrono::time_point<std::chrono::system_clock> &estimatedStart,
                                   const std::chrono::time_point<std::chrono::system_clock> &estimatedFinish,
                                   const std::shared_ptr<LinkAct> &uploadData, enums::dnn_type dnnType, int m, int n,
                                   std::shared_ptr<LinkAct> taskAllocation) : BaseCompResult(
            dnnId, allocatedHost, srcHost, coreAllocation, deadline, estimatedStart, estimatedFinish, uploadData,
            dnnType), M(m), N(n), task_allocation(std::move(taskAllocation)) {}

    HighCompResult::HighCompResult(const std::string &dnnId, const std::string &srcHost,
                                   const std::chrono::time_point<std::chrono::system_clock> &deadline,
                                   const std::shared_ptr<LinkAct> &uploadData, enums::dnn_type dnnType)
            : BaseCompResult(dnnId, srcHost, deadline, uploadData, dnnType) {}

    web::json::value HighCompResult::convertToJson() {
        web::json::value result;

        result["dnn_id"] = web::json::value::string(HighCompResult::getDnnId());
        result["allocated_host"] = web::json::value::string(HighCompResult::getAllocatedHost());
        result["source_host"] = web::json::value::string(HighCompResult::getSrcHost());
        result["core_allocation"] = web::json::value::number(HighCompResult::getM() * HighCompResult::getN());
        result["deadline"] = web::json::value::number(std::chrono::duration_cast<std::chrono::milliseconds>(HighCompResult::getDeadline().time_since_epoch()).count());
        result["estimated_start"] = web::json::value::number(std::chrono::duration_cast<std::chrono::milliseconds>(HighCompResult::getEstimatedStart().time_since_epoch()).count());
        result["estimated_finish"] = web::json::value::number(std::chrono::duration_cast<std::chrono::milliseconds>(HighCompResult::getEstimatedFinish().time_since_epoch()).count());
        result["version"] = web::json::value::number(HighCompResult::getVersion());
        result["N"] = web::json::value::number(HighCompResult::getN());
        result["M"] = web::json::value::number(HighCompResult::getM());
        result["dnn_type"] = web::json::value::string("high_comp");
        return result;
    }

    HighCompResult::HighCompResult(const std::string &dnnId, const std::string &srcHost,
                                   const std::chrono::time_point<std::chrono::system_clock> &deadline,
                                   enums::dnn_type dnnType, int n, int m): BaseCompResult(dnnId, srcHost, deadline, dnnType), M(m), N(n) {

    }
} // model