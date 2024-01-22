//
// Created by Jamie Cotter on 05/02/2023.
//

#include "LowCompResult.h"

namespace model {


    LowCompResult::LowCompResult(const std::string &dnnId, const std::string &srcHost,
                                 int coreAllocation, const std::chrono::time_point<std::chrono::system_clock> &estimatedStart,
                                 const std::chrono::time_point<std::chrono::system_clock> &estimatedFinish,
                                 enums::dnn_type dnnType) : BaseCompResult(
            dnnId, srcHost, coreAllocation, estimatedStart, estimatedFinish,
            dnnType) {}

    web::json::value LowCompResult::convertToJson() {
        web::json::value result;

        result["dnn_id"] = web::json::value::string(LowCompResult::getDnnId());
        result["source_host"] = web::json::value::string(LowCompResult::getSrcHost());
        result["core_allocation"] = web::json::value::number(LowCompResult::getCoreAllocation());
        result["estimated_start"] = web::json::value::number(std::chrono::duration_cast<std::chrono::milliseconds>(
                LowCompResult::getEstimatedStart().time_since_epoch()).count());
        result["estimated_finish"] = web::json::value::number(std::chrono::duration_cast<std::chrono::milliseconds>(
                LowCompResult::getEstimatedFinish().time_since_epoch()).count());
        result["dnn_type"] = web::json::value::string(std::string("low_comp"));
        return result;
    }
} // model