//
// Created by Jamie Cotter on 05/02/2023.
//

#include "LowProcessingItem.h"

#include <utility>

namespace model {

    LowProcessingItem::LowProcessingItem(const std::shared_ptr<std::vector<std::string>> &hostList,
                                         enums::request_type requestType,
                                         std::string sourceDevice, std::string dnnId,
                                         std::chrono::time_point<std::chrono::system_clock> startTime,
                                         std::chrono::time_point<std::chrono::system_clock> finishTime,
                                         bool invokedPreemption) : WorkItem(
            hostList,
            requestType), source_device(std::move(sourceDevice)), dnn_id(std::move(dnnId)), start_time(startTime),
                                                                   finish_time(finishTime),
                                                                   invoked_preemption(invokedPreemption) {}


    const std::string &LowProcessingItem::getDnnId() const {
        return dnn_id;
    }

    const std::string &LowProcessingItem::getSourceDevice() const {
        return source_device;
    }

    const std::chrono::time_point<std::chrono::system_clock> &LowProcessingItem::getStartTime() const {
        return start_time;
    }

    const std::chrono::time_point<std::chrono::system_clock> &LowProcessingItem::getFinishTime() const {
        return finish_time;
    }

    bool LowProcessingItem::isInvokedPreemption() const {
        return invoked_preemption;
    }
} // model