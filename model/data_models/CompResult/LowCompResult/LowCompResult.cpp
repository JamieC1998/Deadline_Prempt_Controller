//
// Created by Jamie Cotter on 05/02/2023.
//

#include "LowCompResult.h"

namespace model {

    const std::string &LowCompResult::getSrcHost() const {
        return srcHost;
    }

    void LowCompResult::setSrcHost(const std::string &srcHost) {
        LowCompResult::srcHost = srcHost;
    }

    const std::chrono::time_point<std::chrono::system_clock> &LowCompResult::getDeadline() const {
        return deadline;
    }

    void LowCompResult::setDeadline(const std::chrono::time_point<std::chrono::system_clock> &deadline) {
        LowCompResult::deadline = deadline;
    }

    const std::chrono::time_point<std::chrono::system_clock> &LowCompResult::getEstimatedStart() const {
        return estimatedStart;
    }

    void LowCompResult::setEstimatedStart(const std::chrono::time_point<std::chrono::system_clock> &estimatedStart) {
        LowCompResult::estimatedStart = estimatedStart;
    }

    const std::chrono::time_point<std::chrono::system_clock> &LowCompResult::getEstimatedFinish() const {
        return estimatedFinish;
    }

    void LowCompResult::setEstimatedFinish(const std::chrono::time_point<std::chrono::system_clock> &estimatedFinish) {
        LowCompResult::estimatedFinish = estimatedFinish;
    }

    const std::shared_ptr<LinkAct> &LowCompResult::getUploadData() const {
        return upload_data;
    }

    void LowCompResult::setUploadData(const std::shared_ptr<LinkAct> &uploadData) {
        upload_data = uploadData;
    }

    LowCompResult::LowCompResult() {}

    LowCompResult::LowCompResult(const std::string &dnnId, enums::dnn_type dnnType, const std::string &srcHost,
                                 const std::chrono::time_point<std::chrono::system_clock> &deadline,
                                 const std::shared_ptr<LinkAct> &uploadData) : BaseCompResult(dnnId, dnnType),
                                                                               srcHost(srcHost), deadline(deadline),
                                                                               upload_data(uploadData) {}

    LowCompResult::LowCompResult(const std::string &dnnId, enums::dnn_type dnnType, const std::string &srcHost,
                                 const std::chrono::time_point<std::chrono::system_clock> &deadline,
                                 const std::chrono::time_point<std::chrono::system_clock> &estimatedStart,
                                 const std::chrono::time_point<std::chrono::system_clock> &estimatedFinish,
                                 const std::shared_ptr<LinkAct> &uploadData) : BaseCompResult(dnnId, dnnType),
                                                                               srcHost(srcHost), deadline(deadline),
                                                                               estimatedStart(estimatedStart),
                                                                               estimatedFinish(estimatedFinish),
                                                                               upload_data(uploadData) {}

    const std::shared_ptr<model::Task> &LowCompResult::getTask() const {
        return task;
    }

    void LowCompResult::setTask(const std::shared_ptr<model::Task> &task) {
        LowCompResult::task = task;
    }

    web::json::value LowCompResult::convertToJson() {
        web::json::value result;
        result[U("dnn_id")] = web::json::value::string(LowCompResult::getDnnId());
        result[U("srcHost")] = web::json::value::string(LowCompResult::getSrcHost());
        result[U("deadline")] = web::json::value::number(std::chrono::duration_cast<std::chrono::milliseconds>(LowCompResult::getDeadline().time_since_epoch()).count());
        result[U("estimatedStart")] = web::json::value::number(std::chrono::duration_cast<std::chrono::milliseconds>(LowCompResult::getEstimatedStart().time_since_epoch()).count());
        result[U("estimatedFinish")] = web::json::value::number(std::chrono::duration_cast<std::chrono::milliseconds>(LowCompResult::getEstimatedFinish().time_since_epoch()).count());
        result[U("task")] = web::json::value(LowCompResult::getTask()->convertToJson());
        result[U("uploadData")] = web::json::value(LowCompResult::getUploadData()->convertToJson());
        return result;
    }

} // model