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
                       std::shared_ptr<model::HighCompResult> dnn, std::string updateConvidx);

        const std::shared_ptr<model::HighCompResult> &getDnn() const;

        void setDnn(const std::shared_ptr<model::HighCompResult> &dnn);

        const std::string &getUpdateConvidx() const;

        void setUpdateConvidx(const std::string &updateConvidx);


    private:
        std::shared_ptr<model::HighCompResult> dnn;
        std::string update_convidx;
    };
};


#endif //CONTROLLER_OUTBOUNDUPDATE_H
