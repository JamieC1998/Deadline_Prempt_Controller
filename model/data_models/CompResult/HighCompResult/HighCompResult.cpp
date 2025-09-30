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
                                   enums::dnn_type dnnType, int m, int n,
                                   std::shared_ptr<NetCommunicationBase> taskAllocation) : BaseCompResult(
            dnnId, allocatedHost, srcHost, coreAllocation, deadline, estimatedStart, estimatedFinish,
            dnnType), M(m), N(n), task_allocation(std::move(taskAllocation)) {}

    HighCompResult::HighCompResult(const std::string &dnnId, const std::string &srcHost,
                                   const std::chrono::time_point<std::chrono::system_clock> &deadline,
                                   enums::dnn_type dnnType)
            : BaseCompResult(dnnId, srcHost, deadline, dnnType) {}

    web::json::value HighCompResult::convertToJson() {
        web::json::value result;

        result["dnn_id"] = web::json::value::string(HighCompResult::getDnnId());
        result["allocated_host"] = web::json::value::string(HighCompResult::getAllocatedHost());
        result["source_host"] = web::json::value::string(HighCompResult::getSrcHost());
        result["core_allocation"] = web::json::value::number(HighCompResult::getCoreAllocation());
        result["deadline"] = web::json::value::number(std::chrono::duration_cast<std::chrono::milliseconds>(HighCompResult::getDeadline().time_since_epoch()).count());
        result["estimated_start"] = web::json::value::number(std::chrono::duration_cast<std::chrono::milliseconds>(HighCompResult::estimated_start_fin->start.time_since_epoch()).count());
        result["estimated_finish"] = web::json::value::number(std::chrono::duration_cast<std::chrono::milliseconds>(HighCompResult::estimated_start_fin->stop.time_since_epoch()).count());
        result["actual_finish"] = web::json::value::number(std::chrono::duration_cast<std::chrono::milliseconds>(HighCompResult::getActualFinish().time_since_epoch()).count());
        if(HighCompResult::getSrcHost() != HighCompResult::getAllocatedHost()) {
            if (task_allocation->type == CommType::contiguous)
                result["task_transfer_data"] = std::static_pointer_cast<model::LinkAct>(
                        HighCompResult::task_allocation)->convertToJson();

            else
                result["task_transfer_data"] = std::static_pointer_cast<model::Bucket>(HighCompResult::task_allocation)->convertToJson();
        }

        result["version"] = web::json::value::number(HighCompResult::getVersion());
        result["N"] = web::json::value::number(HighCompResult::getN());
        result["M"] = web::json::value::number(HighCompResult::getM());
        result["dnn_type"] = web::json::value::string("high_comp");
        return result;
    }
} // model