//
// Created by jamiec on 9/26/22.
//

#ifndef CONTROLLER_CONTROLLERREQUESTPATHS_H
#define CONTROLLER_CONTROLLERREQUESTPATHS_H

namespace constants {
#define HIGH_OFFLOAD_REQUEST "/high_offload_request"
#define LOW_OFFLOAD_REQUEST "/low_offload_request"
#define DEVICE_REGISTER_REQUEST "/register_device"
#define DEADLINE_VIOLATED "/deadline_violation"
#define STATE_UPDATE "/state_update"
#define LOG_RESULT "/log_result"
#define HIGH_WORK_REQUEST "/high_work_request"
#define RETURN_TASK "/ret_task"
#define POST_HALT_TASK "/halt"
#define POST_LOW_TASK "/post_low_task"
#define POST_LOW_COMP_FAIL "/low_comp_fail"
}

#endif //CONTROLLER_CONTROLLERREQUESTPATHS_H
