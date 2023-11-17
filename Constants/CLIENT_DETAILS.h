//
// Created by jamiec on 10/11/22.
//

#ifndef CONTROLLER_CLIENT_DETAILS_H
#define CONTROLLER_CLIENT_DETAILS_H

namespace constant{
#define HIGH_CLIENT_PORT "6501"
#define IPERF_PORT "3114"

/* ENDPOINTS FOR INFERENCE REST */
#define HALT_ENDPOINT "halt"
#define HIGH_TASK_ALLOCATION "allocate_high_task"
#define TASK_UPDATE "update_task"
#define RAISE_CAP "raise_cap"

#define LOW_COMP_PORT "9028"

/* ENDPOINTS FOR EXPERIMENT MANAGER */
#define LOW_TASK_ALLOCATION "allocate_low_task"
#define SET_EXPERIMENT_START "experiment_start"


#define CLIENT_COUNT 1
#define PER_DEVICE_CORES 4
#define MAX_CORES (CLIENT_COUNT * PER_DEVICE_CORES)
#define MAX_CORE_ALLOWANCE PER_DEVICE_CORES

#define CONTROLLER_HOSTNAME "127.0.0.1"
#define CONTROLLER_REST_PORT 6502
}

#endif //CONTROLLER_CLIENT_DETAILS_H
