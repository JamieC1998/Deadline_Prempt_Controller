//
// Created by Jamie Cotter on 26/06/2025.
//

#ifndef CONTROLLER_PROFILERMAIN_H
#define CONTROLLER_PROFILERMAIN_H

#include <chrono>
#include "../../model/data_models/WorkItems/BaseWorkItem/WorkItem.h"
#include "../../model/data_models/NetworkCommsModels/BaseNetworkCommsModel/BaseNetworkCommsModel.h"
#include "../../model/data_models/Network/Network.h"
#include "../../model/data_models/CompResult/HighCompResult/HighCompResult.h"
#include "../../model/data_models/CompResult/LowCompResult/LowCompResult.h"
#include <queue>
#include <memory>
#include "../../model/data_models/SimEvents/SimEvents.h"

namespace services {

	std::map<std::string, std::tuple<int, int, std::shared_ptr<model::TimeWindow>, enums::dnn_type>>
	profiler_event_loop(std::vector<std::string> state_u_list, std::vector<std::shared_ptr<model::SimEvent>> e_queue);

	std::shared_ptr<model::SimEvent>
	handle_inbound(const std::shared_ptr<model::SimEvent> &event, uint64_t &bw_bytes, std::shared_ptr<model::Network> network,
				   std::map<std::string, std::tuple<int, int, std::shared_ptr<model::TimeWindow>, enums::dnn_type>> &resultMap);

	void handle_outbound(const std::shared_ptr<model::SimEvent> &event);

	std::vector<std::shared_ptr<model::SimEvent>>
	handle_work(const std::shared_ptr<model::SimEvent> &event, std::shared_ptr<model::Network> network,
				std::map<std::string, std::shared_ptr<model::BaseCompResult>> &off_total,
				std::map<std::string, std::shared_ptr<model::LowCompResult>> &off_low,
				std::map<std::string, std::shared_ptr<model::HighCompResult>> &off_high, uint64_t bw_bytes,
				std::map<std::string, std::tuple<int, int, std::shared_ptr<model::TimeWindow>, enums::dnn_type>> &resultMap,
				std::vector<std::string> state_u_list);

	int numberOfTasksInNetwork(std::shared_ptr<model::Network> network);

	std::map<std::string, std::tuple<int, int, std::shared_ptr<model::TimeWindow>, enums::dnn_type>>
	printResults(std::map<std::string, std::tuple<int, int, std::shared_ptr<model::TimeWindow>, enums::dnn_type>> resultMap);

	std::pair<std::map<int, std::pair<float, std::vector<uint64_t>>>, std::map<int, std::pair<float, std::vector<uint64_t>>>>
	profile_data_transform(
			std::map<std::string, std::tuple<int, int, std::shared_ptr<model::TimeWindow>, enums::dnn_type>> profile_map);

	void writeTransformedLoadResults(
			std::pair<std::map<int, std::pair<float, std::vector<uint64_t>>>, std::map<int, std::pair<float, std::vector<uint64_t>>>> load_transformation);
} // services

#endif //CONTROLLER_PROFILERMAIN_H
