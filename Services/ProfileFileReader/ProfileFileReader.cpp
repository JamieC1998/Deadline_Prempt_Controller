//
// Created by Jamie Cotter on 21/07/2025.
//
#include "ProfileFileReader.h"
#include <cpprest/json.h>              // JSON library
#include <cpprest/filestream.h>        // File streams
#include <iostream>
#include <fstream>
#include <map>

#include "../../model/enums/LogTypes.h"
#include "../LOG_MANAGER/LogManager.h"
#include "../../utils/UtilFunctions/UtilFunctions.h"

namespace services {

	using namespace utility;               // Common utilities like string conversions
	using namespace web;                   // Common features like URIs
	using namespace web::json;             // JSON library
	using namespace concurrency::streams;

	web::json::value read_experiment_log(const std::string &profileInputPath) {
		try {
			std::ifstream file(profileInputPath);
			if (!file.is_open()) {
				throw std::runtime_error("Could not open file: " + profileInputPath);
			}

			std::stringstream buffer;
			buffer << file.rdbuf();
			std::string json_string = buffer.str();
			file.close();

			return json::value::parse(json_string);
		}
		catch (std::exception e){
			std::cout << e.what() << std::endl;
		}
		return {};
	}

	std::pair<std::vector<std::string>, std::vector<std::shared_ptr<model::SimEvent>>> parse_experiment_log(web::json::value jObj, int device_count) {
		std::vector<std::shared_ptr<model::SimEvent>> resultList;
		std::vector<std::string> state_u_list;

		std::map<std::string, std::vector<std::shared_ptr<model::SimEvent>>> divided_update_map;

		if (jObj.is_array()) {
			auto temp_array = jObj.as_array();

			for (web::json::value event: temp_array) {

				auto e_str = event.serialize();
				auto temp_event = event.as_object();
				auto event_type = temp_event["event_type"].as_string();

				if (event_type == services::fetchEventName(enums::LogTypeEnum::DEVICE_REGISTER)) {
					auto message_content = temp_event["message_content"].as_object();
					auto host = message_content["host"].as_string();
					auto time = temp_event["time"].as_number().to_uint64();
					auto sim_event = std::make_shared<model::InboundEvent>();

					sim_event->i_type = model::InboundEvent::device_register;
					sim_event->type = model::SimEvent::Inbound;
					sim_event->source_device = host;
					sim_event->event_time = std::chrono::system_clock::time_point(std::chrono::milliseconds(time));

					if (divided_update_map.find(host) == divided_update_map.end()) {
						divided_update_map[host] = std::vector<std::shared_ptr<model::SimEvent>>();
					}

					divided_update_map[host].push_back(sim_event);

				} else if (event_type == services::fetchEventName(enums::LogTypeEnum::HIGH_COMP_REQUEST)) {
					auto message_content = temp_event["message_content"].as_object();
					auto sourceDevice = message_content["source_host"].as_string();
					auto dnn_id = message_content["dnn_id"].as_string();
					auto task_count = message_content["task_count"].as_integer();
					auto deadline = message_content["deadline"].as_number().to_uint64();
					auto event_time = temp_event["time"].as_number().to_uint64();
					auto sim_event = std::make_shared<model::HighOffload>();

					sim_event->i_type = model::InboundEvent::high_offload;
					sim_event->type = model::SimEvent::Inbound;
					sim_event->task_count = task_count;
					sim_event->dnn_id = dnn_id;
					sim_event->source_device = sourceDevice;
					sim_event->d_time = std::chrono::system_clock::time_point(std::chrono::milliseconds(deadline));
					sim_event->event_time = std::chrono::system_clock::time_point(std::chrono::milliseconds(event_time));

					if (divided_update_map.find(sourceDevice) == divided_update_map.end()) {
						divided_update_map[sourceDevice] = std::vector<std::shared_ptr<model::SimEvent>>();
					}

					divided_update_map[sourceDevice].push_back(sim_event);

				} else if (event_type == services::fetchEventName(enums::LogTypeEnum::LOW_COMP_REQUEST)) {
					auto message_content = temp_event["message_content"].as_object();
					auto sourceDevice = message_content["source_host"].as_string();
					auto dnn_id = message_content["dnn_id"].as_string();
					auto deadline = message_content["deadline"].as_number().to_uint64();
					auto event_time = temp_event["time"].as_number().to_uint64();
					auto sim_event = std::make_shared<model::InboundEvent>();

					sim_event->i_type = model::InboundEvent::low_offload;
					sim_event->type = model::SimEvent::Inbound;
					sim_event->dnn_id = dnn_id;
					sim_event->source_device = sourceDevice;
					sim_event->d_time = std::chrono::system_clock::time_point(std::chrono::milliseconds(deadline));
					sim_event->event_time = std::chrono::system_clock::time_point(std::chrono::milliseconds(event_time));

					if (divided_update_map.find(sourceDevice) == divided_update_map.end()) {
						divided_update_map[sourceDevice] = std::vector<std::shared_ptr<model::SimEvent>>();
					}

					divided_update_map[sourceDevice].push_back(sim_event);
				} else if (event_type == services::fetchEventName(enums::LogTypeEnum::STATE_UPDATE_REQUEST)) {
					auto message_content = temp_event["message_content"].as_object();
					auto dnn_id = message_content["dnn_id"].as_string();
					auto finish_time = message_content["finish_time"].as_number().to_uint64();
					auto event_time = temp_event["time"].as_number().to_uint64();

					auto text = event.serialize();

					auto sim_event = std::make_shared<model::InboundEvent>();
					sim_event->i_type = model::InboundEvent::state_update;
					sim_event->type = model::SimEvent::Inbound;
					sim_event->dnn_id = dnn_id;
					sim_event->d_time = std::chrono::system_clock::time_point(std::chrono::milliseconds(finish_time));
					sim_event->event_time = std::chrono::system_clock::time_point(std::chrono::milliseconds(event_time));

					auto split_dnn_id = utils::split(dnn_id, "_");
					if (divided_update_map.find(split_dnn_id[0]) == divided_update_map.end()) {
						divided_update_map[split_dnn_id[0]] = std::vector<std::shared_ptr<model::SimEvent>>();
					}


					divided_update_map[split_dnn_id[0]].push_back(sim_event);

				} else if (event_type == services::fetchEventName(enums::LogTypeEnum::VIOLATED_DEADLINE_REQUEST)) {
					auto message_content = temp_event["message_content"].as_object();
					auto dnn_id = message_content["dnn_id"].as_string();
					auto event_time = temp_event["time"].as_number().to_uint64();

					auto sim_event = std::make_shared<model::InboundEvent>();
					sim_event->i_type = model::InboundEvent::deadline_violation;
					sim_event->type = model::SimEvent::Inbound;
					sim_event->dnn_id = dnn_id;
					sim_event->event_time = std::chrono::system_clock::time_point(std::chrono::milliseconds(event_time));

					auto split_dnn_id = utils::split(dnn_id, "_");

					if (divided_update_map.find(split_dnn_id[0]) == divided_update_map.end()) {
						divided_update_map[split_dnn_id[0]] = std::vector<std::shared_ptr<model::SimEvent>>();
					}

					divided_update_map[split_dnn_id[0]].push_back(sim_event);
				} else if (event_type == services::fetchEventName(enums::LogTypeEnum::IPERF_RESULTS)) {
					auto message_content = temp_event["message_content"].as_object();
					auto sim_event = std::make_shared<model::IperfResult>();
					sim_event->bits_per_second = message_content["bits_per_second"].as_number().to_double();
					sim_event->i_type = model::InboundEvent::iperf_result;
					sim_event->jitter = message_content["jitter"].as_number().to_double();
					sim_event->event_time = std::chrono::system_clock::time_point(
							std::chrono::milliseconds(temp_event["time"].as_number().to_uint64()));

					resultList.push_back(sim_event);
				}
			}
		}

		std::map<std::string, std::vector<std::shared_ptr<model::SimEvent>>> duplicated_device_workloads;
		for (auto [sim_event_key, sim_event_list] : divided_update_map) {
			for (int i = 0; i < device_count / divided_update_map.size(); i++) {
				auto new_hostname = sim_event_key + "." + std::to_string(i + 1);

				duplicated_device_workloads[new_hostname] = std::vector<std::shared_ptr<model::SimEvent>>();
				for (const auto& sim_event : sim_event_list) {

					if (sim_event->type == model::SimEvent::Inbound) {
						auto i_e = std::static_pointer_cast<model::InboundEvent>(sim_event);
						auto split_dnn_id = utils::split(i_e->dnn_id, "_");
						auto updated_scoped_hostname = split_dnn_id[0] + "." + std::to_string(i + 1);

						std::string remainder = "";
						for (int k = 1; k < split_dnn_id.size(); k++) {
							remainder += "_" + split_dnn_id[k];
						}

						if (i_e->i_type == model::InboundEvent::high_offload) {
							auto h_i_e = std::static_pointer_cast<model::HighOffload>(i_e);

							auto new_h_i_e = std::make_shared<model::HighOffload>();
							new_h_i_e->i_type = model::InboundEvent::high_offload;
							new_h_i_e->dnn_id = updated_scoped_hostname + remainder;
							new_h_i_e->source_device = new_hostname;
							new_h_i_e->event_time = h_i_e->event_time;
							new_h_i_e->d_time = h_i_e->d_time;
							new_h_i_e->type = h_i_e->type;
							new_h_i_e->task_count = h_i_e->task_count;

							resultList.push_back(new_h_i_e);
						}
						else {
							auto new_i_e = std::make_shared<model::InboundEvent>();
							new_i_e->i_type = i_e->i_type;
							new_i_e->event_time = i_e->event_time;
							new_i_e->d_time = i_e->d_time;
							new_i_e->type = i_e->type;
							new_i_e->dnn_id = updated_scoped_hostname + remainder;
							new_i_e->source_device = new_hostname;

							resultList.push_back(new_i_e);
						}

						if (i_e->i_type == model::InboundEvent::state_update) {
							state_u_list.push_back(updated_scoped_hostname + remainder);
						}

					}
					else if (sim_event->type == model::SimEvent::Outbound) {
						auto o_e = std::static_pointer_cast<model::OutboundWorkItem>(sim_event);

					}
					else if (sim_event->type == model::SimEvent::Work) {
						auto w_e = std::static_pointer_cast<model::WorkQueueEvent>(sim_event);
					}
				}

			}
		}

		std::ranges::sort(resultList, [](const std::shared_ptr<model::SimEvent>& a, const std::shared_ptr<model::SimEvent>& b) {
			return a->event_time < b->event_time;
		});

		return std::make_pair(state_u_list, resultList);
	}
} // services