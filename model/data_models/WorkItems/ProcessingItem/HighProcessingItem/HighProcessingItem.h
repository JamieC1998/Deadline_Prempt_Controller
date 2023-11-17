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


        std::chrono::time_point<std::chrono::system_clock> &getDeadline();

        const std::string &getDnnId() const;

        void setDnnId(const std::string &dnnId);

        std::map<std::string, std::shared_ptr<model::HighCompResult>> baseResult;

    private:
        std::chrono::time_point<std::chrono::system_clock> deadline;
        std::string dnn_id;
    };

} // model

#endif //CONTROLLER_HIGHPROCESSINGITEM_H
