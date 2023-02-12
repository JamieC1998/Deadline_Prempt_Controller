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
                              std::map<std::string, uint64_t> versionMap);

        const std::map<std::string, uint64_t> &getVersionMap() const;

    private:
        /* Key is dnn_id, value is version */
        std::map<std::string, uint64_t> version_map;
    };
};


#endif //CONTROLLER_HALTNETWORKCOMMSMODEL_H