//
// Created by Jamie Cotter on 05/02/2023.
//

#ifndef CONTROLLER_BASECOMPRESULT_H
#define CONTROLLER_BASECOMPRESULT_H

#include <string>
#include "../../../enums/DNN_Type_Enum.h"
#include "../../LinkAct/LinkAct.h"

namespace model {

    class BaseCompResult {
        static int uniqueDnnIdCounter;
    public:


        explicit BaseCompResult(enums::dnn_type dnnType);

        BaseCompResult(std::string dnnId, std::string allocatedHost, std::string srcHost,
                       int coreAllocation, const std::chrono::time_point<std::chrono::system_clock> &deadline,
                       const std::chrono::time_point<std::chrono::system_clock> &estimatedStart,
                       const std::chrono::time_point<std::chrono::system_clock> &estimatedFinish,
                       std::shared_ptr<LinkAct> uploadData, enums::dnn_type dnnType);

        BaseCompResult(std::string dnnId, std::string srcHost, int coreAllocation,
                       const std::chrono::time_point<std::chrono::system_clock> &deadline,
                       const std::chrono::time_point<std::chrono::system_clock> &estimatedStart,
                       const std::chrono::time_point<std::chrono::system_clock> &estimatedFinish,
                       std::shared_ptr<LinkAct> uploadData, enums::dnn_type dnnType
        );

        BaseCompResult(std::string dnnId, std::string srcHost,
                       const std::chrono::time_point<std::chrono::system_clock> &deadline,
                       std::shared_ptr<LinkAct> uploadData, enums::dnn_type dnnType);

        BaseCompResult(std::string dnnId, std::string srcHost,
                       const std::chrono::time_point<std::chrono::system_clock> &deadline,
                       enums::dnn_type dnnType);

        BaseCompResult(std::string dnnId, std::string srcHost,
                       const std::chrono::time_point<std::chrono::system_clock> &deadline,
                       enums::dnn_type dnnType, int core_allocation);

        int getUniqueDnnId() const;

        const std::string &getDnnId() const;

        void setDnnId(const std::string &dnnId);

        const std::string &getAllocatedHost() const;

        void setAllocatedHost(const std::string &allocatedHost);

        const std::string &getSrcHost() const;

        void setSrcHost(const std::string &srcHost);

        int getCoreAllocation() const;

        void setCoreAllocation(int coreAllocation);

        const std::chrono::time_point<std::chrono::system_clock> &getDeadline() const;

        void setDeadline(const std::chrono::time_point<std::chrono::system_clock> &deadline);

        const std::chrono::time_point<std::chrono::system_clock> &getEstimatedStart() const;

        void setEstimatedStart(const std::chrono::time_point<std::chrono::system_clock> &estimatedStart);

        const std::chrono::time_point<std::chrono::system_clock> &getEstimatedFinish() const;

        void setEstimatedFinish(const std::chrono::time_point<std::chrono::system_clock> &estimatedFinish);

        const std::shared_ptr<LinkAct> &getUploadData() const;

        void setUploadData(const std::shared_ptr<LinkAct> &uploadData);

        enums::dnn_type getDnnType() const;

        void setDnnType(enums::dnn_type dnnType);

        const std::chrono::time_point<std::chrono::system_clock> &getActualFinish() const;

        void setActualFinish(const std::chrono::time_point<std::chrono::system_clock> &actualFinish);

        const std::shared_ptr<LinkAct> &getStateUpdate() const;

        void setStateUpdate(const std::shared_ptr<LinkAct> &stateUpdate);

    private:
        int uniqueDnnId{};
        std::string dnn_id;
        std::string allocated_host;
        std::string srcHost;

        int core_allocation = 0;
        std::chrono::time_point<std::chrono::system_clock> deadline;
        std::chrono::time_point<std::chrono::system_clock> estimatedStart;
        std::chrono::time_point<std::chrono::system_clock> estimatedFinish;

        std::chrono::time_point<std::chrono::system_clock> actualFinish;

        std::shared_ptr<LinkAct> upload_data;
        std::shared_ptr<LinkAct> state_update;
        enums::dnn_type dnnType;
    };

} // model

#endif //CONTROLLER_BASECOMPRESULT_H
