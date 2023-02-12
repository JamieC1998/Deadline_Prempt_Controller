//
// Created by jamiec on 9/27/22.
//

#include "HighCompResult.h"

#include <utility>

namespace model {
    const std::string &HighCompResult::getSrcHost() const {
        return srcHost;
    }

    void HighCompResult::setSrcHost(const std::string &srcHost) {
        HighCompResult::srcHost = srcHost;
    }

    std::chrono::time_point<std::chrono::system_clock> HighCompResult::getDeadline() const {
        return deadline;
    }

    void HighCompResult::setDeadline(std::chrono::time_point<std::chrono::system_clock> deadline) {
        HighCompResult::deadline = deadline;
    }

    std::chrono::time_point<std::chrono::system_clock> HighCompResult::getEstimatedStart() const {
        return estimatedStart;
    }

    void HighCompResult::setEstimatedStart(std::chrono::time_point<std::chrono::system_clock> estimatedStart) {
        HighCompResult::estimatedStart = estimatedStart;
    }

    std::chrono::time_point<std::chrono::system_clock> HighCompResult::getEstimatedFinish() const {
        return estimatedFinish;
    }

    void HighCompResult::setEstimatedFinish(std::chrono::time_point<std::chrono::system_clock> estimatedFinish) {
        HighCompResult::estimatedFinish = estimatedFinish;
    }

    const std::string &HighCompResult::getStartingConvidx() const {
        return starting_convidx;
    }

    void HighCompResult::setStartingConvidx(const std::string &startingConvidx) {
        starting_convidx = startingConvidx;
    }

    HighCompResult::HighCompResult(const std::string &dnnId, enums::dnn_type dnnType, std::string srcHost,
                                   const std::chrono::time_point<std::chrono::system_clock> &deadline,
                                   const std::chrono::time_point<std::chrono::system_clock> &estimatedStart,
                                   std::string startingConvidx, std::shared_ptr<LinkAct> uploadData)
            : BaseCompResult(dnnId, dnnType), srcHost(std::move(srcHost)), deadline(deadline),
              estimatedStart(estimatedStart),
              starting_convidx(std::move(startingConvidx)), upload_data(std::move(uploadData)) {}

    const std::shared_ptr<LinkAct> &HighCompResult::getUploadData() const {
        return upload_data;
    }

    void HighCompResult::setUploadData(const std::shared_ptr<LinkAct> &uploadData) {
        upload_data = uploadData;
    }

    void HighCompResult::resetUploadData() {
        upload_data.reset();
    }

    web::json::value HighCompResult::convertToJson() {
        web::json::value result_json;
        result_json["unique_dnn_id"] = web::json::value::number(HighCompResult::getUniqueDnnId());
        result_json["dnnId"] = web::json::value::string(HighCompResult::getDnnId());
        result_json["srcHost"] = web::json::value::string(HighCompResult::getSrcHost());

        result_json["deadline"] = web::json::value::number(
                std::chrono::duration_cast<std::chrono::milliseconds>(HighCompResult::getDeadline().time_since_epoch()).count());

        result_json["estimatedStart"] = web::json::value::number(
                std::chrono::duration_cast<std::chrono::milliseconds>(HighCompResult::getEstimatedStart().time_since_epoch()).count());

        result_json["estimatedFinish"] = web::json::value::number(
                std::chrono::duration_cast<std::chrono::milliseconds>(HighCompResult::getEstimatedFinish().time_since_epoch()).count());

        result_json["startingConvidx"] = web::json::value::string(HighCompResult::getStartingConvidx());
        result_json["lastCompleteConvidx"] = web::json::value::number(HighCompResult::getLastCompleteConvIdx());
        result_json["version"] = web::json::value::number(HighCompResult::getVersion());

        web::json::value upload_data_json;

        if (upload_data)
            upload_data_json = HighCompResult::getUploadData()->convertToJson();

        result_json["uploadData"] = upload_data_json;

        web::json::value tasks_json;

        for (const auto &task_pair : HighCompResult::tasks) {
            tasks_json[task_pair.first] = task_pair.second->convertToJson();
        }
        result_json["tasks"] = tasks_json;

        return result_json;
    }

    void HighCompResult::setLastCompleteConvIdx(int currentConvix) {
        HighCompResult::lastCompleteConvidx = currentConvix;
    }

    int HighCompResult::getLastCompleteConvIdx() const {
        return HighCompResult::lastCompleteConvidx;
    }

    uint64_t HighCompResult::getVersion() {
        return version;
    }

    void HighCompResult::setVersion(uint64_t version) {
        HighCompResult::version = version;
    }
} // model