//
// Created by Jamie Cotter on 07/02/2023.
//

#ifndef CONTROLLER_OUTBOUNDUPDATE_H
#define CONTROLLER_OUTBOUNDUPDATE_H

#include "../BaseNetworkCommsModel/BaseNetworkCommsModel.h"
#include "../../CompResult/HighCompResult/HighCompResult.h"


namespace model {
    class OutboundUpdate: public BaseNetworkCommsModel {
    public:
        OutboundUpdate(enums::network_comms_types type,
                       const std::chrono::time_point<std::chrono::system_clock> &commTime,
                       std::shared_ptr<model::HighCompResult> dnn, std::string updateConvidx, uint64_t old_version);

        const std::shared_ptr<model::HighCompResult> &getDnn() const;

        void setDnn(const std::shared_ptr<model::HighCompResult> &dnn);

        const std::string &getUpdateConvidx() const;

        void setUpdateConvidx(const std::string &updateConvidx);

        uint64_t getOldVersion() const;

        void setOldVersion(uint64_t oldVersion);

    private:
        std::shared_ptr<model::HighCompResult> dnn;
        std::string update_convidx;
        uint64_t old_version;
    };
};


#endif //CONTROLLER_OUTBOUNDUPDATE_H
