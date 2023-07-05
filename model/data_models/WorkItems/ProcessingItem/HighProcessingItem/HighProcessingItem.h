//
// Created by jamiec on 9/26/22.
//

#ifndef CONTROLLER_HIGHPROCESSINGITEM_H
#define CONTROLLER_HIGHPROCESSINGITEM_H

#include <map>
#include <memory>
#include "../../BaseWorkItem/WorkItem.h"
#include "../../../CompResult/HighCompResult/HighCompResult.h"

namespace model {
    //Low Comp, High Comp and DL_Prempt
    class HighProcessingItem : public WorkItem {
    public:
        HighProcessingItem(enums::request_type requestType, std::shared_ptr<std::vector<std::string>> &hostList);

        HighProcessingItem(std::shared_ptr<std::vector<std::string>> &hostList, enums::request_type requestType,
                           std::chrono::time_point<std::chrono::system_clock> deadline, std::string dnnId);

        HighProcessingItem(std::shared_ptr<std::vector<std::string>> &hostList, enums::request_type requestType,
                           std::chrono::time_point<std::chrono::system_clock> deadline, std::string dnnId,
                           std::vector<std::string> dnnIds);

        std::chrono::time_point<std::chrono::system_clock> &getDeadline();

        const std::string &getDnnId() const;

        void setDnnId(const std::string &dnnId);

        std::map<std::string, std::shared_ptr<model::HighCompResult>> baseResult;

        const std::vector<std::string> &getDnnIds() const;

    private:
        std::chrono::time_point<std::chrono::system_clock> deadline;
        std::string base_dnn_id;

        std::vector<std::string> dnn_ids;
    };

} // model

#endif //CONTROLLER_HIGHPROCESSINGITEM_H
