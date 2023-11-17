//
// Created by Jamie Cotter on 10/11/2023.
//

#ifndef CONTROLLER_WORKREQUEST_H
#define CONTROLLER_WORKREQUEST_H

#include "../BaseWorkItem/WorkItem.h"

namespace model {

    class WorkRequest: public WorkItem{
    public:
        WorkRequest(const std::shared_ptr<std::vector<std::string>> &hostList, enums::request_type requestType,
                    int capacity);

        int getCapacity() const;

        void setCapacity(int capacity);

    private:
        int capacity = 0;
    };

} // model

#endif //CONTROLLER_WORKREQUEST_H
