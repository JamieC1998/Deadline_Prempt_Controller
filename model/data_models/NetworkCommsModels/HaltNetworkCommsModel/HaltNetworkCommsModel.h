//
// Created by Jamie Cotter on 09/02/2023.
//

#ifndef CONTROLLER_HALTNETWORKCOMMSMODEL_H
#define CONTROLLER_HALTNETWORKCOMMSMODEL_H


#include <map>
#include "../BaseNetworkCommsModel/BaseNetworkCommsModel.h"

namespace model{
    class HaltNetworkCommsModel: public BaseNetworkCommsModel {
    public:
        HaltNetworkCommsModel(enums::network_comms_types type,
                              const std::chrono::time_point<std::chrono::system_clock> &commTime,
                              const std::string &hostToContact, const std::string &dnnId, uint64_t versionNumber);

        const std::string &getHostToContact() const;

        const std::string &getDnnId() const;

        uint64_t getVersionNumber() const;

    private:
        std::string hostToContact;
        std::string dnnId;
        uint64_t versionNumber;
    };
};


#endif //CONTROLLER_HALTNETWORKCOMMSMODEL_H
