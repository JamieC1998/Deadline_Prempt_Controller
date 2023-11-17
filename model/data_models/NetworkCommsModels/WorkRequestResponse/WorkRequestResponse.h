//
// Created by Jamie Cotter on 12/11/2023.
//

#ifndef CONTROLLER_WORKREQUESTRESPONSE_H
#define CONTROLLER_WORKREQUESTRESPONSE_H

#include "../BaseNetworkCommsModel/BaseNetworkCommsModel.h"

namespace model {

    class WorkRequestResponse : public BaseNetworkCommsModel {
    public:
        WorkRequestResponse(enums::network_comms_types type,
                            const std::chrono::time_point<std::chrono::system_clock> &commTime, bool isSuccess,
                            int coreCapacity, std::string contactDevice);

        const std::string &getContactDevice() const;

        bool isSuccess() const;

        int getCoreCapacity() const;

    private:
        std::string contact_device;
        bool is_success{};
        int core_capacity{};
    };

} // model

#endif //CONTROLLER_WORKREQUESTRESPONSE_H
