//
// Created by Jamie Cotter on 05/02/2023.
//

#ifndef CONTROLLER_BASECOMPRESULT_H
#define CONTROLLER_BASECOMPRESULT_H

#include <string>
#include "../../../enums/DNN_Type_Enum.h"

namespace model {

    class BaseCompResult {
    public:
        BaseCompResult(const std::string &dnnId, enums::dnn_type dnnType);
        BaseCompResult();

        const std::string &getDnnId() const;

        void setDnnId(const std::string &dnnId);

        enums::dnn_type getDnnType() const;

        void setDnnType(enums::dnn_type dnnType);

    private:
        std::string dnn_id;
        enums::dnn_type dnnType;
    };

} // model

#endif //CONTROLLER_BASECOMPRESULT_H
