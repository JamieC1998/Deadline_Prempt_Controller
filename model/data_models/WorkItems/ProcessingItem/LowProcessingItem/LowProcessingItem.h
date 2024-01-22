//
// Created by Jamie Cotter on 05/02/2023.
//

#ifndef CONTROLLER_LOWPROCESSINGITEM_H
#define CONTROLLER_LOWPROCESSINGITEM_H

#include <map>
#include "../../BaseWorkItem/WorkItem.h"

namespace model {

    class LowProcessingItem : public WorkItem {
    public:
        LowProcessingItem(const std::shared_ptr<std::vector<std::string>> &hostList, enums::request_type requestType,
                          std::string dnnId, std::string sourceDevice,
                          std::chrono::time_point<std::chrono::system_clock> startTime,
                          std::chrono::time_point<std::chrono::system_clock> finishTime, bool invokePreemption);

        const std::string &getDnnId() const;

        const std::string &getSourceDevice() const;

        const std::chrono::time_point<std::chrono::system_clock> &getStartTime() const;

        const std::chrono::time_point<std::chrono::system_clock> &getFinishTime() const;

        bool isInvokedPreemption() const;

    private:
        /* The key to both of these is the dnn_id */
        std::string dnn_id;
        std::string source_device;
        std::chrono::time_point<std::chrono::system_clock> start_time;
        std::chrono::time_point<std::chrono::system_clock> finish_time;
        bool invoked_preemption;
    };

} // model

#endif //CONTROLLER_LOWPROCESSINGITEM_H
