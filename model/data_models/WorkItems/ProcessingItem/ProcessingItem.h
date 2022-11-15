//
// Created by jamiec on 9/26/22.
//

#ifndef CONTROLLER_PROCESSINGITEM_H
#define CONTROLLER_PROCESSINGITEM_H

#include <map>
#include <memory>
#include "../BaseWorkItem/WorkItem.h"
#include "../../BaseDNNModel/BaseDNNModel.h"

namespace model {
    //Low Comp, High Comp and DL_Prempt
    class ProcessingItem: WorkItem {
    public:
        ProcessingItem(enums::request_type requestType, const std::shared_ptr<std::vector<std::string>> &hostList,
                       std::map<std::string, std::shared_ptr<BaseDNNModel>> allocationInputData);

        const std::map<std::string, std::shared_ptr<BaseDNNModel>> &getAllocationInputData() const;

        void setAllocationInputData(const std::map<std::string, std::shared_ptr<BaseDNNModel>> &allocationInputData);

        std::shared_ptr<BaseDNNModel> getItem(std::string key);

        const std::map<std::string, std::chrono::time_point<std::chrono::high_resolution_clock>> &getDeadline() const;

        const std::map<std::string, std::string> &getInputPath() const;

        void setInputPath(const std::map<std::string, std::string> &inputPath);
    private:
        std::map<std::string, std::shared_ptr<BaseDNNModel>> allocation_input_data;
        std::map<std::string, std::chrono::time_point<std::chrono::high_resolution_clock>> deadline;
        std::map<std::string, std::string> input_path;
    };

} // model

#endif //CONTROLLER_PROCESSINGITEM_H
