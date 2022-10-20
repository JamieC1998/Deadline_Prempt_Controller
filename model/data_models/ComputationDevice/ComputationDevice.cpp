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

    int ComputationDevice::getActiveCores() const {
        return active_cores;
    }

    void ComputationDevice::setActiveCores(int activeCores) {
        active_cores = activeCores;
    }

    float ComputationDevice::getRam() const {
        return ram;
    }

    void ComputationDevice::setRam(float ram) {
        ComputationDevice::ram = ram;
    }

    float ComputationDevice::getActiveRam() const {
        return active_ram;
    }

    void ComputationDevice::setActiveRam(float activeRam) {
        active_ram = activeRam;
    }

    float ComputationDevice::getStorage() const {
        return storage;
    }

    void ComputationDevice::setStorage(float storage) {
        ComputationDevice::storage = storage;
    }

    float ComputationDevice::getActiveStorage() const {
        return active_storage;
    }

    void ComputationDevice::setActiveStorage(float activeStorage) {
        active_storage = activeStorage;
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

    ComputationDevice::ComputationDevice(int cores, float ram, float storage, const std::string &hostName) : cores(
            cores), ram(ram), storage(storage), host_name(hostName), id(id_counter){ id_counter++; }

    int ComputationDevice::getId() const {
        return id;
    }
} // model