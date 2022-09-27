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
        BaseResult(int sourceDevId, enums::dnn_type dnnType, const std::string &srcHost, time_t deadline,
                   time_t estimatedStart, time_t estimatedFinish);

        int getSourceDevId() const;

        void setSourceDevId(int sourceDevId);

        enums::dnn_type getDnnType() const;

        void setDnnType(enums::dnn_type dnnType);

        const std::string &getSrcHost() const;

        void setSrcHost(const std::string &srcHost);

        time_t getDeadline() const;

        void setDeadline(time_t deadline);

        time_t getEstimatedStart() const;

        void setEstimatedStart(time_t estimatedStart);

        time_t getEstimatedFinish() const;

        void setEstimatedFinish(time_t estimatedFinish);

        int getDnnId() const;

        void setDnnId(int dnnId);

    private:
        int dnn_id;
        int sourceDevId;
        enums::dnn_type dnnType;
        std::string srcHost;
        time_t deadline;
        time_t estimatedStart;
        time_t estimatedFinish;
        int block_count;
        std::map<int, std::map<int, std::shared_ptr<Task>>> tasks;
    };

} // model

#endif //CONTROLLER_BASERESULT_H
