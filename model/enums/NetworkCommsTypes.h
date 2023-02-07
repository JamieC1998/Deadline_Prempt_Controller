//
// Created by jamiec on 10/11/22.
//

#ifndef CONTROLLER_NETWORKCOMMSTYPES_H
#define CONTROLLER_NETWORKCOMMSTYPES_H

namespace enums{
    enum class network_comms_types { halt_req = 0, task_update = 1, low_complexity_allocation = 2, high_complexity_task_mapping = 3, high_complexity_task_reallocation = 4 };
}

#endif //CONTROLLER_NETWORKCOMMSTYPES_H
