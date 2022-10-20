//
// Created by jamiec on 9/23/22.
//

#ifndef CONTROLLER_REQUESTTYPEENUM_H
#define CONTROLLER_REQUESTTYPEENUM_H

#include "../../Constants/RequestTypes.h"

using namespace constant;
namespace enums {
    enum class request_type : int { low_complexity=LOW_COMPLEXITY, high_complexity=HIGH_COMPLEXITY, dag_disruption=DAG_DISRUPTION, deadline_prempt=DEADLINE_PREEMPTION, halt_req=HALT_REQ, state_update=STATE_UPDATE};
}

#endif //CONTROLLER_REQUESTTYPEENUM_H
