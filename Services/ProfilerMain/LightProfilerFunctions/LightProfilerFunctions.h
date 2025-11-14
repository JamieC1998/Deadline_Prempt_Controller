//
// Created by Jamie Cotter on 23/10/2025.
//

#ifndef LIGHTPROFILERFUNCTIONS_H
#define LIGHTPROFILERFUNCTIONS_H

#include "../../../model/data_models/WorkItems/ProcessingItem/LowProcessingItem/LowProcessingItem.h"
#include "../../LightSchedulingFunctions/LightSchedulingFunctions.h"
#include "../../../model/data_models/NetworkCommsModels/LowComplexityAllocation/LowComplexityAllocationComms.h"
#include "../../../model/data_models/WorkItems/HaltWorkItem/HaltWorkItem.h"
#include "../../../model/data_models/SimEvents/SimEvents.h"
#include "../../../model/data_models/Network/Network.h"
#include "../../../model/data_models/WorkItems/StateUpdate/StateUpdate.h"
#include "../../../utils/ProfilerUtils/ProfilerUtils.h"

namespace services {
	std::shared_ptr<model::Network>
	update_network_disc_prof(uint64_t bps, std::shared_ptr<model::Network> network);

	std::tuple<std::map<std::string, std::shared_ptr<model::HighCompResult> >, std::map<std::string, std::shared_ptr<model::LowCompResult> >, std::map<
			std::string, std::shared_ptr<model::BaseCompResult> >, std::map<std::string, std::tuple<int, int, std::shared_ptr<model::TimeWindow>,
			enums::dnn_type> >
		, std::vector<std::shared_ptr<model::SimEvent> >, std::shared_ptr<model::Network> >
	light_work_function_low_comp(uint64_t bw_bytes, std::shared_ptr<model::Network> network, std::string sourceHost,
								 std::shared_ptr<model::WorkQueueEvent> w_event,
								 std::vector<std::shared_ptr<model::SimEvent> > resultVect, std::shared_ptr<model::SimEvent> event,
								 std::map<std::string, std::shared_ptr<model::BaseCompResult> > off_total,
								 std::map<std::string, std::shared_ptr<model::LowCompResult> > off_low,
								 std::map<std::string, std::shared_ptr<model::HighCompResult> > off_high,
								 std::map<std::string, std::tuple<int, int, std::shared_ptr<model::TimeWindow>, enums::dnn_type> > resultMap,
								 std::shared_ptr<model::LowProcessingItem> l_proc);

	std::tuple<std::map<std::string, std::shared_ptr<model::HighCompResult> >, std::map<std::string, std::shared_ptr<model::LowCompResult> >, std::map<
			std::string, std::shared_ptr<model::BaseCompResult> >, std::map<std::string, std::tuple<int, int, std::shared_ptr<model::TimeWindow>,
			enums::dnn_type> >
		, std::vector<std::shared_ptr<model::SimEvent> >, std::shared_ptr<model::Network>, std::vector<std::shared_ptr<model::NetCommunicationBase> >, std::map<
			std::string, std::shared_ptr<model::ComputationDevice> > >
	light_work_function_high_comp(std::map<std::string, std::shared_ptr<model::ComputationDevice> > copyDeviceWorkloadList,
								  std::shared_ptr<model::HighProcessingItem> h_proc,
								  std::map<std::string, std::shared_ptr<model::BaseCompResult> > off_total,
								  std::map<std::string, std::shared_ptr<model::LowCompResult> > off_low,
								  std::map<std::string, std::shared_ptr<model::HighCompResult> > off_high,
								  std::vector<std::shared_ptr<model::NetCommunicationBase> > copyList, std::string sourceHost, uint64_t bw_bytes,
								  std::vector<std::string> state_u_list,
								  std::shared_ptr<model::Network> network,
								  std::map<std::string, std::tuple<int, int, std::shared_ptr<model::TimeWindow>, enums::dnn_type> > resultMap,
								  std::vector<std::shared_ptr<model::SimEvent> > resultVect);
} // services

#endif //LIGHTPROFILERFUNCTIONS_H
