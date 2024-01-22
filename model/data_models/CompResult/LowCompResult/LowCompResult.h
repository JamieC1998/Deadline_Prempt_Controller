//
// Created by Jamie Cotter on 05/02/2023.
//

#ifndef CONTROLLER_LOWCOMPRESULT_H
#define CONTROLLER_LOWCOMPRESULT_H

#include <string>
#include "../../LinkAct/LinkAct.h"
#include "../BaseCompResult/BaseCompResult.h"

namespace model {

    class LowCompResult: public BaseCompResult {

    public:

        LowCompResult(const std::string &dnnId, const std::string &allocatedHost,
                      int coreAllocation, const std::chrono::time_point<std::chrono::system_clock> &estimatedStart,
                      const std::chrono::time_point<std::chrono::system_clock> &estimatedFinish,
                      enums::dnn_type dnnType);


        web::json::value convertToJson();

    };

} // model

#endif //CONTROLLER_LOWCOMPRESULT_H
