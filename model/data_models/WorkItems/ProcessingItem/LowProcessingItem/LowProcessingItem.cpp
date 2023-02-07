//
// Created by Jamie Cotter on 05/02/2023.
//

#include "LowProcessingItem.h"

#include <utility>

namespace model {

    const std::chrono::time_point<std::chrono::system_clock> &
    LowProcessingItem::getDeadline() const {
        return deadline;
    }

    void LowProcessingItem::setDeadline(
            std::chrono::time_point<std::chrono::system_clock> &deadline) {
        LowProcessingItem::deadline = deadline;
    }

    LowProcessingItem::LowProcessingItem(const std::shared_ptr<std::vector<std::string>> &hostList,
                                         enums::request_type requestType,
                                         std::chrono::time_point<std::chrono::system_clock> deadline,
                                         std::pair<std::string, std::string> dnnIdsAndDevice) : WorkItem(hostList,
                                                                                                         requestType),
                                                                                                deadline(std::move(
                                                                                                        deadline)),
                                                                                                dnn_ids_and_device(
                                                                                                        std::move(
                                                                                                                dnnIdsAndDevice)) {}

    const std::pair<std::string, std::string> &LowProcessingItem::getDnnIdAndDevice() const {
        return dnn_ids_and_device;
    }

    void LowProcessingItem::setDnnIdsAndDevice(const std::pair<std::string, std::string> &dnnIdsAndDevice) {
        dnn_ids_and_device = dnnIdsAndDevice;
    }
} // model