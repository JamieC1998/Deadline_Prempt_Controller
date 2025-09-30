//
// Created by Jamie Cotter on 01/07/2025.
//

#ifndef CONTROLLER_HEAVYSCHEDULINGFUNCTIONS_H
#define CONTROLLER_HEAVYSCHEDULINGFUNCTIONS_H

#include "../../model/data_models/CompResult/BaseCompResult/BaseCompResult.h"
#include "../../model/data_models/CompResult/HighCompResult/HighCompResult.h"
#include "../../model/data_models/CompResult/LowCompResult/LowCompResult.h"
#include "../../model/data_models/ComputationDevice/ComputationDevice.h"
#include "../../model/data_models/LinkAct/LinkAct.h"
#include "../../model/data_models/Network/Network.h"
#include "../../model/data_models/WorkItems/BaseWorkItem/WorkItem.h"
#include "../../model/data_models/WorkItems/ProcessingItem/HighProcessingItem/HighProcessingItem.h"
#include "../../model/data_models/WorkItems/HaltWorkItem/HaltWorkItem.h"

#include <map>
#include <memory>

namespace services {

    void state_update_function(std::map<std::string, std::shared_ptr<model::BaseCompResult>> &off_total,
                               std::map<std::string, std::shared_ptr<model::LowCompResult>> &off_low,
                               std::map<std::string, std::shared_ptr<model::HighCompResult>> &off_high,
                               std::string dnn_id,
                               std::shared_ptr<model::BaseCompResult> dnn,
                               std::shared_ptr<model::ComputationDevice> device,
                               std::vector<std::shared_ptr<model::NetCommunicationBase>> &network_link_list,
                               bool isSuccess,
                               std::chrono::time_point<std::chrono::system_clock> finish_time
    );

    std::pair<std::shared_ptr<model::LowCompResult>, std::shared_ptr<model::TimeWindow>>
    low_comp_allocation_func(uint64_t bw_bytes,
                             std::shared_ptr<model::Network> network,
                             std::string host, std::shared_ptr<model::WorkItem> item);

    std::vector<std::shared_ptr<model::HighCompResult>>
    high_comp_allocation_func(bool isReallocation, std::shared_ptr<model::HighProcessingItem> processingItem,
                              std::map<std::string, std::shared_ptr<model::BaseCompResult>> &off_total,
                              std::map<std::string, std::shared_ptr<model::ComputationDevice>> copyDeviceWorkloadList,
                              std::vector<std::shared_ptr<model::NetCommunicationBase>> copyList,
                              uint64_t bw_bytes,
                              std::string sourceHost);

	std::tuple<std::shared_ptr<model::BaseCompResult>,
			std::vector<std::shared_ptr<model::BaseCompResult>>,
			std::vector<std::shared_ptr<model::NetCommunicationBase>>,
			std::map<std::string, std::shared_ptr<model::BaseCompResult>>,
			std::map<std::string, std::shared_ptr<model::HighCompResult>>, uint64_t>
	halt_call_func(std::shared_ptr<model::HaltWorkItem> haltWorkItem,
				   std::vector<std::shared_ptr<model::BaseCompResult>> deviceTaskLoad,
				   std::vector<std::shared_ptr<model::NetCommunicationBase>> copyList,
				   std::map<std::string, std::shared_ptr<model::BaseCompResult>> off_total,
				   std::map<std::string, std::shared_ptr<model::HighCompResult>> off_high);
} // services

#endif //CONTROLLER_HEAVYSCHEDULINGFUNCTIONS_H
