//
// Created by Jamie Cotter on 06/02/2023.
//

#ifndef CONTROLLER_LOWCOMPLEXITYALLOCATIONCOMMS_H
#define CONTROLLER_LOWCOMPLEXITYALLOCATIONCOMMS_H

#include <memory>
#include "../../CompResult/LowCompResult/LowCompResult.h"
#include "../BaseNetworkCommsModel/BaseNetworkCommsModel.h"

namespace model {

    class LowComplexityAllocationComms: public BaseNetworkCommsModel {
    public:
        LowComplexityAllocationComms(enums::network_comms_types type,
                                     const std::chrono::time_point<std::chrono::system_clock> &commTime,
                                     std::shared_ptr<LowCompResult> allocatedTask, std::string host);

        const std::shared_ptr<LowCompResult> &getAllocatedTask() const;

        void setAllocatedTask(const std::shared_ptr<LowCompResult> &allocatedTask);

        const std::string &getHost() const;

        void setHost(const std::string &host);

    private:
        std::shared_ptr<LowCompResult> allocated_task;
        std::string host;
    };

} // model

#endif //CONTROLLER_LOWCOMPLEXITYALLOCATIONCOMMS_H
