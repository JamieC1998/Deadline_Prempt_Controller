//
// Created by jamiec on 9/27/22.
//

#ifndef CONTROLLER_COMPUTATIONDEVICE_H
#define CONTROLLER_COMPUTATIONDEVICE_H

#include <string>
#include <vector>
#include <memory>
#include "../CompResult/BaseCompResult/BaseCompResult.h"

namespace model {

    class ComputationDevice {
        static int id_counter;

    public:
        ComputationDevice(int cores, std::string hostName);

        int getCores() const;

        void setCores(int cores);

        const std::string &getHostName() const;

        void setHostName(const std::string &hostName);

        const std::vector<std::shared_ptr<BaseCompResult>> &getDNNs() const;

        void setTasks(const std::vector<std::shared_ptr<BaseCompResult>> &tasks);

        int getId() const;

        std::vector<std::shared_ptr<BaseCompResult>> DNNS;

        web::json::value convertToJson();

        const std::string &getLastLowCompId() const;

        void setLastLowCompId(const std::string &lastLowCompId);

    private:
        int id;
        int cores;
        std::string host_name;
        std::string last_low_comp_id = "N/A";
    };

} // model

#endif //CONTROLLER_COMPUTATIONDEVICE_H
