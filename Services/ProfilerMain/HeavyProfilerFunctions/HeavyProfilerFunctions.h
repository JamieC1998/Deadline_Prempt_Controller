//
// Created by Jamie Cotter on 21/10/2025.
//


#ifndef HEAVYPROFILERFUNCTIONS_H
#define HEAVYPROFILERFUNCTIONS_H

#include "../../HeavySchedulingFunctions/HeavySchedulingFunctions.h"
#include "../../../model/data_models/SimEvents/SimEvents.h"
#include "../../../model/data_models/WorkItems/ProcessingItem/LowProcessingItem/LowProcessingItem.h"
#include "../../../utils/ProfilerUtils/ProfilerUtils.h"
#include "../../../model/data_models/NetworkCommsModels/LowComplexityAllocation/LowComplexityAllocationComms.h"

namespace services {
	std::tuple<std::map<std::string, std::shared_ptr<model::HighCompResult> >, std::map<std::string, std::shared_ptr<model::LowCompResult> >, std::map<
			std::string, std::shared_ptr<model::BaseCompResult> >, std::map<std::string, std::tuple<int, int, std::shared_ptr<model::TimeWindow>,
			enums::dnn_type> >
		, std::vector<std::shared_ptr<model::SimEvent> >, std::shared_ptr<model::Network> >
	work_function_low_comp(uint64_t bw_bytes, std::shared_ptr<model::Network> network, std::string sourceHost,
						   std::shared_ptr<model::WorkQueueEvent> w_event,
						   std::vector<std::shared_ptr<model::SimEvent> > resultVect, std::shared_ptr<model::SimEvent> event,
						   std::map<std::string, std::shared_ptr<model::BaseCompResult> > off_total,
						   std::map<std::string, std::shared_ptr<model::LowCompResult> > off_low,
						   std::map<std::string, std::shared_ptr<model::HighCompResult> > off_high,
						   std::map<std::string, std::tuple<int, int, std::shared_ptr<model::TimeWindow>, enums::dnn_type> > resultMap);

	std::tuple<std::map<std::string, std::shared_ptr<model::HighCompResult> >, std::map<std::string, std::shared_ptr<model::LowCompResult> >, std::map<
			std::string, std::shared_ptr<model::BaseCompResult> >, std::map<std::string, std::tuple<int, int, std::shared_ptr<model::TimeWindow>,
			enums::dnn_type> >
		, std::vector<std::shared_ptr<model::SimEvent> >, std::shared_ptr<model::Network>, std::vector<std::shared_ptr<model::NetCommunicationBase> >, std::map<
			std::string, std::shared_ptr<model::ComputationDevice> > >
	work_function_high_comp(std::map<std::string, std::shared_ptr<model::ComputationDevice> > copyDeviceWorkloadList,
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

#endif //HEAVYPROFILERFUNCTIONS_H
