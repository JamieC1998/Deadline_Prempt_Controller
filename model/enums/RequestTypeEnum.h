//
// Created by jamiec on 9/23/22.
//

#ifndef CONTROLLER_REQUESTTYPEENUM_H
#define CONTROLLER_REQUESTTYPEENUM_H

#include "../../Constants/RequestTypes.h"

using namespace constant;
namespace enums {
    enum class request_type : int { low_complexity=LOW_COMPLEXITY, prune_dnn=PRUNE_DNN, high_complexity=HIGH_COMPLEXITY, dag_disruption=DAG_DISRUPTION, halt_req=HALT_REQ, state_update=REQ_TYPE_STATE_UPDATE};
}

#endif //CONTROLLER_REQUESTTYPEENUM_H
