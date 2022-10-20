//
// Created by jamiec on 10/4/22.
//

#ifndef CONTROLLER_COMMSENUMS_H
#define CONTROLLER_COMMSENUMS_H

#include "../../Constants/CommTypes.h"

namespace enums{
    enum class CommsEnums : int { task_mapping = TASK_MAPPING, task_allocation = TASK_ALLOCATION, task_update = TASK_UPDATE, halt_message = HALT_REQUEST };
}

#endif //CONTROLLER_COMMSENUMS_H
