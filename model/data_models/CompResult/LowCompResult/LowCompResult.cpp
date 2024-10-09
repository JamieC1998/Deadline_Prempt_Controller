//
// Created by Jamie Cotter on 05/02/2023.
//

#include "LowCompResult.h"

namespace model {


    LowCompResult::LowCompResult(const std::string &dnnId, const std::string &allocatedHost, const std::string &srcHost,
                                 int coreAllocation, const std::chrono::time_point<std::chrono::system_clock> &deadline,
                                 const std::chrono::time_point<std::chrono::system_clock> &estimatedStart,
                                 const std::chrono::time_point<std::chrono::system_clock> &estimatedFinish,
                                 enums::dnn_type dnnType) : BaseCompResult(
            dnnId, allocatedHost, srcHost, coreAllocation, deadline, estimatedStart, estimatedFinish,
            dnnType) {}

    LowCompResult::LowCompResult(const std::string &dnnId, const std::string &srcHost, int coreAllocation,
                                 const std::chrono::time_point<std::chrono::system_clock> &deadline,
                                 const std::chrono::time_point<std::chrono::system_clock> &estimatedStart,
                                 const std::chrono::time_point<std::chrono::system_clock> &estimatedFinish,
                                 enums::dnn_type dnnType) : BaseCompResult(
            dnnId, srcHost, coreAllocation, deadline, estimatedStart, estimatedFinish, dnnType) {}

    web::json::value LowCompResult::convertToJson() {
        web::json::value result;

        result["dnn_id"] = web::json::value::string(LowCompResult::getDnnId());
        result["source_host"] = web::json::value::string(LowCompResult::getSrcHost());
        result["core_allocation"] = web::json::value::number(LowCompResult::getCoreAllocation());
        result["deadline"] = web::json::value::number(std::chrono::duration_cast<std::chrono::milliseconds>(LowCompResult::getDeadline().time_since_epoch()).count());
        result["estimated_start"] = web::json::value::number(std::chrono::duration_cast<std::chrono::milliseconds>(LowCompResult::estimated_start_fin->start.time_since_epoch()).count());
        result["estimated_finish"] = web::json::value::number(std::chrono::duration_cast<std::chrono::milliseconds>(LowCompResult::estimated_start_fin->stop.time_since_epoch()).count());
        result["actual_finish"] = web::json::value::number(std::chrono::duration_cast<std::chrono::milliseconds>(LowCompResult::getActualFinish().time_since_epoch()).count());
        result["dnn_type"] = web::json::value::string(std::string ("low_comp"));
        return result;
    }
} // model