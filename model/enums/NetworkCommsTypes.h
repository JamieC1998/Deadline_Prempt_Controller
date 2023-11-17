//
// Created by jamiec on 10/11/22.
//

#ifndef CONTROLLER_NETWORKCOMMSTYPES_H
#define CONTROLLER_NETWORKCOMMSTYPES_H

namespace enums{
    enum class network_comms_types { initial_experiment_start = 0, halt_req = 1, task_update = 2, low_complexity_allocation = 3, high_complexity_task_mapping = 4, high_complexity_task_reallocation = 5, prune_dnn = 6, work_request_response = 7 };
}

#endif //CONTROLLER_NETWORKCOMMSTYPES_H
