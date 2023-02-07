//
// Created by jamiec on 9/27/22.
//

#include "ComputationDevice.h"

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

    const std::vector<std::shared_ptr<Task>> &ComputationDevice::getTasks() const {
        return TASKS;
    }

    void ComputationDevice::setTasks(const std::vector<std::shared_ptr<Task>> &tasks) {
        TASKS = tasks;
    }

    ComputationDevice::ComputationDevice(int cores, const std::string &hostName) : cores(
            cores), host_name(hostName), id(id_counter){ id_counter++; }

    int ComputationDevice::getId() const {
        return id;
    }

} // model