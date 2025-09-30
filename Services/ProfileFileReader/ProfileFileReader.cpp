//
// Created by Jamie Cotter on 21/07/2025.
//
#include "ProfileFileReader.h"
#include <cpprest/json.h>              // JSON library
#include <cpprest/filestream.h>        // File streams
#include <iostream>
#include <memory>
#include <fstream>
#include "../../model/enums/LogTypes.h"
#include "../../services/LOG_MANAGER/LogManager.h"

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
			std::cerr << e.what() << std::endl;
		}
		return {};
	}

	std::pair<std::vector<std::string>, std::vector<std::shared_ptr<model::SimEvent>>> parse_experiment_log(web::json::value jObj) {
		std::vector<std::shared_ptr<model::SimEvent>> resultList;
		std::vector<std::string> state_u_list;
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

					resultList.push_back(sim_event);
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

					resultList.push_back(sim_event);
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

					resultList.push_back(sim_event);
				} else if (event_type == services::fetchEventName(enums::LogTypeEnum::STATE_UPDATE_REQUEST)) {
					auto message_content = temp_event["message_content"].as_object();
					auto dnn_id = message_content["dnn_id"].as_string();
					auto finish_time = message_content["finish_time"].as_number().to_uint64();
					auto event_time = temp_event["time"].as_number().to_uint64();

					state_u_list.push_back(dnn_id);

					auto sim_event = std::make_shared<model::InboundEvent>();
					sim_event->i_type = model::InboundEvent::state_update;
					sim_event->type = model::SimEvent::Inbound;
					sim_event->dnn_id = dnn_id;
					sim_event->d_time = std::chrono::system_clock::time_point(std::chrono::milliseconds(finish_time));
					sim_event->event_time = std::chrono::system_clock::time_point(std::chrono::milliseconds(event_time));

					resultList.push_back(sim_event);
				} else if (event_type == services::fetchEventName(enums::LogTypeEnum::VIOLATED_DEADLINE_REQUEST)) {
					auto message_content = temp_event["message_content"].as_object();
					auto dnn_id = message_content["dnn_id"].as_string();
					auto event_time = temp_event["time"].as_number().to_uint64();

					auto sim_event = std::make_shared<model::InboundEvent>();
					sim_event->i_type = model::InboundEvent::deadline_violation;
					sim_event->type = model::SimEvent::Inbound;
					sim_event->dnn_id = dnn_id;
					sim_event->event_time = std::chrono::system_clock::time_point(std::chrono::milliseconds(event_time));

					resultList.push_back(sim_event);
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

		return std::make_pair(state_u_list, resultList);
	}
} // services