//
// Created by Jamie Cotter on 04/02/2023.
//

#ifndef CONTROLLER_HIGHCOMPLEXITYALLOCATIONCOMMS_H
#define CONTROLLER_HIGHCOMPLEXITYALLOCATIONCOMMS_H

#include "../../CompResult/HighCompResult/HighCompResult.h"
#include "../BaseNetworkCommsModel/BaseNetworkCommsModel.h"

namespace model {

    class HighComplexityAllocationComms: public BaseNetworkCommsModel {

    public:
        HighComplexityAllocationComms(enums::network_comms_types type,
                                      const std::chrono::time_point<std::chrono::system_clock> &commTime,
                                      std::shared_ptr<HighCompResult> allocatedTask, std::string host);

        const std::shared_ptr<HighCompResult> &getAllocatedTask() const;

        void setAllocatedTask(const std::shared_ptr<HighCompResult> &allocatedTask);

        const std::string &getHost() const;

        void setHost(const std::string &host);

    private:
        //Allocation is true
        std::shared_ptr<HighCompResult> allocatedTask;
        std::string host;
    };

} // model

#endif //CONTROLLER_HIGHCOMPLEXITYALLOCATIONCOMMS_H
