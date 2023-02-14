//
// Created by jamiec on 10/11/22.
//

#ifndef CONTROLLER_CLIENT_DETAILS_H
#define CONTROLLER_CLIENT_DETAILS_H

namespace constant{
#define HIGH_CLIENT_PORT "3113"
#define IPERF_PORT "3114"

/* ENDPOINTS FOR INFERENCE REST */
#define HALT_ENDPOINT "halt"
#define HIGH_TASK_ALLOCATION "allocate_high_task"
#define HIGH_TASK_REALLOCATION "reallocate_high_task"
#define TASK_UPDATE "update_task"

#define LOW_COMP_PORT "1337"

/* ENDPOINTS FOR EXPERIMENT MANAGER */
#define LOW_TASK_ALLOCATION "allocate_low_task"
#define SET_EXPERIMENT_START "experiment_start"

#define PER_DEVICE_CORES 4
#define MAX_CORES 16
#define MAX_CORE_ALLOWANCE (MAX_CORES / 2)
#define CLIENT_COUNT 4
#define CONTROLLER_HOSTNAME "COM-C132-M34579"
}

#endif //CONTROLLER_CLIENT_DETAILS_H
