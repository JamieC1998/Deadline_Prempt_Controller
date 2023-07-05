//
// Created by Jamie Cotter on 05/02/2023.
//

#include "LowCompResult.h"

namespace model {


    LowCompResult::LowCompResult(const std::string &dnnId, const std::string &allocatedHost, const std::string &srcHost,
                                 int coreAllocation, const std::chrono::time_point<std::chrono::system_clock> &deadline,
                                 const std::chrono::time_point<std::chrono::system_clock> &estimatedStart,
                                 const std::chrono::time_point<std::chrono::system_clock> &estimatedFinish,
                                 const std::shared_ptr<LinkAct> &uploadData, enums::dnn_type dnnType) : BaseCompResult(
            dnnId, allocatedHost, srcHost, coreAllocation, deadline, estimatedStart, estimatedFinish, uploadData,
            dnnType) {}

    LowCompResult::LowCompResult(const std::string &dnnId, const std::string &srcHost, int coreAllocation,
                                 const std::chrono::time_point<std::chrono::system_clock> &deadline,
                                 const std::chrono::time_point<std::chrono::system_clock> &estimatedStart,
                                 const std::chrono::time_point<std::chrono::system_clock> &estimatedFinish,
                                 const std::shared_ptr<LinkAct> &uploadData, enums::dnn_type dnnType) : BaseCompResult(
            dnnId, srcHost, coreAllocation, deadline, estimatedStart, estimatedFinish, uploadData, dnnType) {}

    web::json::value LowCompResult::convertToJson() {
        web::json::value result;

        result["dnn_id"] = web::json::value::string(LowCompResult::getDnnId());
        result["source_host"] = web::json::value::string(LowCompResult::getSrcHost());
        result["core_allocation"] = web::json::value::number(LowCompResult::getCoreAllocation());
        result["deadline"] = web::json::value::number(std::chrono::duration_cast<std::chrono::milliseconds>(LowCompResult::getDeadline().time_since_epoch()).count());
        result["estimated_start"] = web::json::value::number(std::chrono::duration_cast<std::chrono::milliseconds>(LowCompResult::getEstimatedStart().time_since_epoch()).count());
        result["estimated_finish"] = web::json::value::number(std::chrono::duration_cast<std::chrono::milliseconds>(LowCompResult::getEstimatedFinish().time_since_epoch()).count());
        result["actual_finish"] = web::json::value::number(std::chrono::duration_cast<std::chrono::milliseconds>(LowCompResult::getActualFinish().time_since_epoch()).count());
        result["upload_data"] = LowCompResult::getUploadData()->convertToJson();
        result["dnn_type"] = web::json::value::string(std::string ("low_comp"));
        return result;
    }
} // model