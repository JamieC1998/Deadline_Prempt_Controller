//
// Created by jamiec on 9/23/22.
//

#ifndef CONTROLLER_WORKITEM_H
#define CONTROLLER_WORKITEM_H

#include <vector>
#include <string>
#include <memory>
#include "../../../enums/RequestTypeEnum.h"


namespace model {

    class WorkItem {
    public:
        static int internal_item_id_counter;

        int internal_id;

        enums::request_type getRequestType() const;

        WorkItem();

        explicit WorkItem(enums::request_type requestType);

        WorkItem(std::shared_ptr<std::vector<std::string>> hostList, enums::request_type requestType);

        void setRequestType(enums::request_type requestType);

        std::shared_ptr<std::vector<std::string>> &getHostList();

    private:
        std::shared_ptr<std::vector<std::string>> host_list;
        enums::request_type requestType;
    };

} // model

#endif //CONTROLLER_WORKITEM_H
