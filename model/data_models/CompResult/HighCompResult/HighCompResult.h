//
// Created by jamiec on 9/27/22.
//

#ifndef CONTROLLER_HIGHCOMPRESULT_H
#define CONTROLLER_HIGHCOMPRESULT_H

#include <ctime>
#include <string>
#include <map>
#include "../../../enums/DNN_Type_Enum.h"
#include "../BaseCompResult/BaseCompResult.h"
#include "../../LinkAct/LinkAct.h"
#include "../../Bucket/Bucket.h"
#include "../../../NetCommunicationBase/NetCommunicationBase.h"

namespace model {

    class HighCompResult : public BaseCompResult {
    public:

        HighCompResult(const std::string &dnnId, const std::string &allocatedHost, const std::string &srcHost,
                       int coreAllocation, const std::chrono::time_point<std::chrono::system_clock> &deadline,
                       const std::chrono::time_point<std::chrono::system_clock> &estimatedStart,
                       const std::chrono::time_point<std::chrono::system_clock> &estimatedFinish,
                       enums::dnn_type dnnType, int m, int n,
                       std::shared_ptr<NetCommunicationBase> taskAllocation);

        HighCompResult(const std::string &dnnId, const std::string &srcHost,
                       const std::chrono::time_point<std::chrono::system_clock> &deadline,
                       enums::dnn_type dnnType);

        web::json::value convertToJson();

        int getM() const;

        void setM(int m);

        int getN() const;

        void setN(int n);

        uint64_t getVersion() const;

        void setVersion(uint64_t version);

        std::shared_ptr<NetCommunicationBase> task_allocation;

    private:
        int M = 0;
        int N = 0;

        uint64_t version = std::chrono::system_clock::now().time_since_epoch().count() * 1000;
    };

} // model

#endif //CONTROLLER_HIGHCOMPRESULT_H
