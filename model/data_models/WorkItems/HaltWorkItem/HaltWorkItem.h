//
// Created by Jamie Cotter on 28/04/2023.
//

#ifndef CONTROLLER_HALTWORKITEM_H
#define CONTROLLER_HALTWORKITEM_H

#include "../BaseWorkItem/WorkItem.h"

namespace model {

    class HaltWorkItem : public WorkItem {
    public:
        HaltWorkItem(enums::request_type requestType, std::string alloHost, std::string dnn_id);

        const std::string &getAllocatedHost() const;

        const std::string &getDnnId() const;

    private:
        std::string allocated_host;
        std::string dnn_id;
    };

} // model

#endif //CONTROLLER_HALTWORKITEM_H
