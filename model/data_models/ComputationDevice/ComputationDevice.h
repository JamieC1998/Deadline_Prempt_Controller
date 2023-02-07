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
        ComputationDevice(int cores, const std::string &hostName);

        int getCores() const;

        void setCores(int cores);

        const std::string &getHostName() const;

        void setHostName(const std::string &hostName);

        const std::vector<std::shared_ptr<Task>> &getTasks() const;

        void setTasks(const std::vector<std::shared_ptr<Task>> &tasks);

        int getId() const;

        std::vector<std::shared_ptr<Task>> TASKS;

    private:
        int id;
        int cores;
        std::string host_name;
    };

} // model

#endif //CONTROLLER_COMPUTATIONDEVICE_H
