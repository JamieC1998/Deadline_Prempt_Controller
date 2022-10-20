//
// Created by jamiec on 9/27/22.
//

#ifndef CONTROLLER_COMPUTATIONDEVICE_H
#define CONTROLLER_COMPUTATIONDEVICE_H

#include <string>
#include <vector>
#include <memory>
#include "../Task/Task.h"

namespace model {

    class ComputationDevice {
        static int id_counter;

    public:
        ComputationDevice(int cores, float ram, float storage, const std::string &hostName);

        int getCores() const;

        void setCores(int cores);

        int getActiveCores() const;

        void setActiveCores(int activeCores);

        float getRam() const;

        void setRam(float ram);

        float getActiveRam() const;

        void setActiveRam(float activeRam);

        float getStorage() const;

        void setStorage(float storage);

        float getActiveStorage() const;

        void setActiveStorage(float activeStorage);

        const std::string &getHostName() const;

        void setHostName(const std::string &hostName);

        const std::vector<std::shared_ptr<Task>> &getTasks() const;

        void setTasks(const std::vector<std::shared_ptr<Task>> &tasks);

        int getId() const;

    private:
        int id;
        int cores;
        int active_cores;
        float ram;
        float active_ram;
        float storage;
        float active_storage;
        std::string host_name;
        std::vector<std::shared_ptr<Task>> TASKS;
    };

} // model

#endif //CONTROLLER_COMPUTATIONDEVICE_H
