//
// Created by jamiec on 9/27/22.
//

#include "ComputationDevice.h"
#include "../CompResult/HighCompResult/HighCompResult.h"
#include "../CompResult/LowCompResult/LowCompResult.h"

#include <utility>

namespace model {
    int ComputationDevice::id_counter = 0;

    int ComputationDevice::getCores() const {
        return cores;
    }

    void ComputationDevice::setCores(int cores) {
        ComputationDevice::cores = cores;
    }

    const std::string &ComputationDevice::getHostName() const {
        return host_name;
    }

    void ComputationDevice::setHostName(const std::string &hostName) {
        host_name = hostName;
    }

    const std::vector<std::shared_ptr<BaseCompResult>> &ComputationDevice::getDNNs() const {
        return DNNS;
    }

    void ComputationDevice::setTasks(const std::vector<std::shared_ptr<BaseCompResult>> &tasks) {
        DNNS = tasks;
    }

    ComputationDevice::ComputationDevice(int cores, std::string hostName) : cores(
            cores), host_name(std::move(hostName)), id(id_counter){ id_counter++; }

    int ComputationDevice::getId() const {
        return id;
    }

    web::json::value ComputationDevice::convertToJson(){
        web::json::value result;
        result["host_name"] = web::json::value::string(ComputationDevice::getHostName());

        std::vector<web::json::value> dnn_list;
        for(const auto& dnn_task: ComputationDevice::DNNS){
            if(dnn_task->getDnnType() == enums::dnn_type::high_comp)
                dnn_list.push_back(std::static_pointer_cast<HighCompResult>(dnn_task)->convertToJson());
            else
                dnn_list.push_back(std::static_pointer_cast<LowCompResult>(dnn_task)->convertToJson());
        }
        result["tasks"] = web::json::value::array(dnn_list);
        return result;
    };

} // model