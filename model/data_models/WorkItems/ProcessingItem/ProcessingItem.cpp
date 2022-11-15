//
// Created by jamiec on 9/26/22.
//

#include "ProcessingItem.h"

#include <utility>

namespace model {
    const std::map<std::string, std::shared_ptr<BaseDNNModel>> & ProcessingItem::getAllocationInputData() const {
        return allocation_input_data;
    }

    std::shared_ptr<BaseDNNModel> ProcessingItem::getItem(std::string key) {
        return allocation_input_data[key];
    }


    void ProcessingItem::setAllocationInputData(const std::map<std::string, std::shared_ptr<BaseDNNModel>> &allocationInputData) {
        allocation_input_data = allocationInputData;
    }

    ProcessingItem::ProcessingItem(enums::request_type requestType, const std::shared_ptr<std::vector<std::string>> &hostList,
                                   std::map<std::string, std::shared_ptr<BaseDNNModel>> allocationInputData) : WorkItem(
            requestType, hostList), allocation_input_data(std::move(allocationInputData)) {}

    const std::map<std::string, std::chrono::time_point<std::chrono::high_resolution_clock>> &
    ProcessingItem::getDeadline() const {
        return deadline;
    }

    const std::map<std::string, std::string> &ProcessingItem::getInputPath() const {
        return input_path;
    }

    void ProcessingItem::setInputPath(const std::map<std::string, std::string> &inputPath) {
        input_path = inputPath;
    }
} // model