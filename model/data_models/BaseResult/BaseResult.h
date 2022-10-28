//
// Created by jamiec on 9/27/22.
//

#ifndef CONTROLLER_BASERESULT_H
#define CONTROLLER_BASERESULT_H

#include <ctime>
#include <string>
#include <map>
#include "../../enums/DNN_Type_Enum.h"
#include "../Task/Task.h"

namespace model {

    class BaseResult {
        static int dnnIdCounter;

    public:
        BaseResult(int sourceDevId, enums::dnn_type dnnType, const std::string &srcHost, std::chrono::time_point<std::chrono::system_clock> deadline,
                   std::chrono::time_point<std::chrono::system_clock> estimatedStart, std::chrono::time_point<std::chrono::system_clock> estimatedFinish);

        int getSourceDevId() const;

        void setSourceDevId(int sourceDevId);

        enums::dnn_type getDnnType() const;

        void setDnnType(enums::dnn_type dnnType);

        const std::string &getSrcHost() const;

        void setSrcHost(const std::string &srcHost);

        std::chrono::time_point<std::chrono::system_clock> getDeadline() const;

        void setDeadline(std::chrono::time_point<std::chrono::system_clock> deadline);

        std::chrono::time_point<std::chrono::system_clock> getEstimatedStart() const;

        void setEstimatedStart(std::chrono::time_point<std::chrono::system_clock> estimatedStart);

        std::chrono::time_point<std::chrono::system_clock> getEstimatedFinish() const;

        void setEstimatedFinish(std::chrono::time_point<std::chrono::system_clock> estimatedFinish);

        int getDnnId() const;

        void setDnnId(int dnnId);

        std::map<int, std::map<int, std::shared_ptr<Task>>> tasks;
        std::pair<int, int> current_block = std::make_pair<int, int>(0, 0);

    private:
        int dnn_id;
        int sourceDevId;

        enums::dnn_type dnnType;
        std::string srcHost;
        std::chrono::time_point<std::chrono::system_clock> deadline;
        std::chrono::time_point<std::chrono::system_clock> estimatedStart;
        std::chrono::time_point<std::chrono::system_clock> estimatedFinish;
        int group_block_count;

    };

} // model

#endif //CONTROLLER_BASERESULT_H
