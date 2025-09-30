//
// Created by Jamie Cotter on 21/07/2025.
//

#ifndef CONTROLLER_SIMEVENTS_H
#define CONTROLLER_SIMEVENTS_H

#include <chrono>
#include "../WorkItems/BaseWorkItem/WorkItem.h"
#include "../NetworkCommsModels/BaseNetworkCommsModel/BaseNetworkCommsModel.h"

namespace model{

    struct SimEvent {
        enum EType {
            Inbound, Work, Outbound
        } type;
        std::chrono::time_point<std::chrono::system_clock> event_time;
    };

    struct InboundEvent : SimEvent {
        enum IeType {
            high_offload, low_offload, state_update, deadline_violation, device_register, iperf_result
        } i_type;
        std::string dnn_id;
        std::string source_device;
        std::chrono::time_point<std::chrono::system_clock> d_time;
    };

    struct HighOffload : InboundEvent {
        int task_count = 0;
    };

	struct IperfResult : InboundEvent {
		double bits_per_second = 0.0;
		double jitter = 0.0;
	};

    struct WorkQueueEvent : SimEvent {
        std::shared_ptr<model::WorkItem> work_item;
    };

    struct OutboundWorkItem : SimEvent {
        std::shared_ptr<model::BaseNetworkCommsModel> network_work_item;
    };

    struct SimEventCompare {
        bool operator()(const std::shared_ptr<SimEvent> &a, const std::shared_ptr<SimEvent> &b) const {
            return a->event_time > b->event_time;  // earlier time = higher priority
        }
    };
}

#endif //CONTROLLER_SIMEVENTS_H
