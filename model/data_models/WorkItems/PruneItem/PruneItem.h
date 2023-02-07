//
// Created by Jamie Cotter on 06/02/2023.
//

#ifndef CONTROLLER_PRUNEITEM_H
#define CONTROLLER_PRUNEITEM_H

#include <string>
#include "../BaseWorkItem/WorkItem.h"

namespace model {

    class PruneItem: public WorkItem{
    public:
        PruneItem(enums::request_type requestType, std::string dnnId, std::string nextConvBlock);

        const std::string &getDnnId() const;

        void setDnnId(const std::string &dnnId);

        const std::string &getNextConvBlock() const;

        void setNextConvBlock(const std::string &nextConvBlock);

    private:
        std::string dnn_id;
        std::string next_conv_block;
    };

} // model

#endif //CONTROLLER_PRUNEITEM_H
