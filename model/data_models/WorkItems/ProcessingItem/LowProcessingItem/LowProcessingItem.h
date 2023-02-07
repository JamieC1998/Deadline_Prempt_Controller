//
// Created by Jamie Cotter on 05/02/2023.
//

#ifndef CONTROLLER_LOWPROCESSINGITEM_H
#define CONTROLLER_LOWPROCESSINGITEM_H

#include <map>
#include "../../BaseWorkItem/WorkItem.h"

namespace model {

    class LowProcessingItem: public WorkItem{
    public:
        LowProcessingItem(const std::shared_ptr<std::vector<std::string>> &hostList, enums::request_type requestType,
                          std::chrono::time_point<std::chrono::system_clock> deadline,
                          std::pair<std::string, std::string> dnnIdsAndDevice);

        const std::chrono::time_point<std::chrono::system_clock> &getDeadline() const;

        void setDeadline(std::chrono::time_point<std::chrono::system_clock> &deadline);

        const std::pair<std::string, std::string> &getDnnIdAndDevice() const;

        void setDnnIdsAndDevice(const std::pair<std::string, std::string> &dnnIdsAndDevice);

    private:
        /* The key to both of these is the dnn_id */
        std::chrono::time_point<std::chrono::system_clock> deadline;
        std::pair<std::string, std::string> dnn_ids_and_device;
    };

} // model

#endif //CONTROLLER_LOWPROCESSINGITEM_H
