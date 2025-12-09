//
// Created by Jamie Cotter on 26/06/2025.
//

#include "ProfilerMain.h"

#include <utility>

#include "../../model/data_models/WorkItems/ProcessingItem/HighProcessingItem/HighProcessingItem.h"
#include "../../model/data_models/WorkItems/ProcessingItem/LowProcessingItem/LowProcessingItem.h"
#include "../../model/data_models/NetworkCommsModels/LowComplexityAllocation/LowComplexityAllocationComms.h"
#include "../../model/data_models/NetworkCommsModels/HaltNetworkCommsModel/HaltNetworkCommsModel.h"
#include "../../model/data_models/WorkItems/HaltWorkItem/HaltWorkItem.h"
#include "../../model/data_models/WorkItems/StateUpdate/StateUpdate.h"
#include "../HeavySchedulingFunctions/HeavySchedulingFunctions.h"
#include "../../model/data_models/NetworkCommsModels/HighComplexityAllocation/HighComplexityAllocationComms.h"
#include "../../Constants/CLIENT_DETAILS.h"
#include "../../Constants/NETWORK_DISC_PARAMS.h"
#include "../../Constants/LOG_CONSTANT.h"
#include "../../utils/ProfilerUtils/ProfilerUtils.h"
#include "HeavyProfilerFunctions/HeavyProfilerFunctions.h"
#include "LightProfilerFunctions/LightProfilerFunctions.h"

namespace services {
	std::shared_ptr<model::SimEvent>
	handle_inbound(const std::shared_ptr<model::SimEvent> &event, uint64_t &bw_bytes, std::shared_ptr<model::Network> network,
	               std::map<std::string, std::tuple<int, int, std::pair<std::chrono::time_point<std::chrono::steady_clock>,std::chrono::time_point<std::chrono::steady_clock>>, enums::dnn_type, int> > &resultMap, int e_queue_size) {
		std::chrono::time_point<std::chrono::system_clock> start = std::chrono::system_clock::now();

		std::shared_ptr<model::InboundEvent> i_event = std::static_pointer_cast<model::InboundEvent>(event);
		std::shared_ptr<model::SimEvent> return_event;
		if (i_event->i_type == model::InboundEvent::high_offload) {
			std::shared_ptr<model::HighOffload> h_event = std::static_pointer_cast<model::HighOffload>(i_event);
			std::vector<std::string> dnnIds;

			dnnIds.reserve(h_event->task_count);

			int tasks_in_network = utils::numberOfTasksInNetwork(network);
			for (int i = 0; i < h_event->task_count; i++) {
				auto dnn_id = h_event->dnn_id + "_" + std::to_string(i);
				dnnIds.push_back(dnn_id);

				if (resultMap.find(dnn_id) == resultMap.end()) {
					auto tw = std::make_pair(std::chrono::steady_clock::now(),
					                                              std::chrono::steady_clock::now());
					resultMap[dnn_id] = std::make_tuple(tasks_in_network, -1, tw, enums::dnn_type::high_comp, e_queue_size);
				} else {
					auto [beginningTaskCount, finTaskCount, window, dnnType, e_q] = resultMap[dnn_id];
					window.first = std::chrono::steady_clock::now();
					window.second = std::chrono::steady_clock::now();
					resultMap[dnn_id] = std::make_tuple(tasks_in_network, finTaskCount, window, dnnType, e_queue_size);
				}
			}


			auto hostList = std::make_shared<std::vector<std::string> >(
				std::initializer_list<std::string>{h_event->source_device});
			std::shared_ptr<model::HighProcessingItem> h_proc = std::make_shared<model::HighProcessingItem>(
				hostList, enums::request_type::high_complexity,
				h_event->d_time, h_event->dnn_id, dnnIds);

			auto res = std::make_shared<model::WorkQueueEvent>();
			res->work_item = h_proc;
			res->type = model::SimEvent::Work;
			return_event = std::static_pointer_cast<model::SimEvent>(res);
		} else if (i_event->i_type == model::InboundEvent::low_offload) {
			std::string dnn_id = i_event->dnn_id;
			std::pair<std::string, std::string> dnnIdsAndDevice = std::make_pair(dnn_id, i_event->source_device);
			std::shared_ptr<std::vector<std::string> > hostList = std::make_shared<std::vector<std::string> >(
				std::initializer_list{i_event->source_device});
			std::shared_ptr<model::LowProcessingItem> l_proc = std::make_shared<model::LowProcessingItem>(hostList,
				enums::request_type::low_complexity,
				i_event->d_time,
				dnnIdsAndDevice);

			int tasks_in_network = utils::numberOfTasksInNetwork(network);
			if (resultMap.find(dnn_id) == resultMap.end()) {
				auto tw = std::make_pair(std::chrono::steady_clock::now(), std::chrono::steady_clock::now());
				resultMap[dnn_id] = std::make_tuple(tasks_in_network, -1, tw, enums::dnn_type::low_comp, e_queue_size);
			} else {
				auto [beginningTaskCount, finTaskCount, window, dnnType, e_q] = resultMap[dnn_id];
				window.first = std::chrono::steady_clock::now();
				window.second = std::chrono::steady_clock::now();
				resultMap[dnn_id] = std::make_tuple(tasks_in_network, finTaskCount, window, dnnType, e_queue_size);
			}

			auto res = std::make_shared<model::WorkQueueEvent>();
			res->work_item = l_proc;
			res->type = model::SimEvent::Work;
			return_event = std::static_pointer_cast<model::SimEvent>(res);
		} else if (i_event->i_type == model::InboundEvent::state_update || i_event->i_type == model::InboundEvent::deadline_violation) {
			std::string dnn_id = i_event->dnn_id;

			auto hostList = std::make_shared<std::vector<std::string> >(
				std::initializer_list<std::string>{i_event->source_device});

			std::shared_ptr<model::StateUpdate> s_update = std::make_shared<model::StateUpdate>(hostList,
			                                                                                    enums::request_type::state_update,
			                                                                                    i_event->d_time,
			                                                                                    i_event->dnn_id);
			if (i_event->i_type != model::InboundEvent::state_update)
				s_update->setSuccess(false);

			auto res = std::make_shared<model::WorkQueueEvent>();
			res->work_item = s_update;
			res->type = model::SimEvent::Work;
			return_event = std::static_pointer_cast<model::SimEvent>(res);
		} else if (i_event->i_type == model::InboundEvent::iperf_result) {
			auto temp_event = std::static_pointer_cast<model::IperfResult>(i_event);
			/* Fetching the bandwidth and latency */
			double bw_bytes_per_second = temp_event->bits_per_second / 8;
			double latency_bytes = temp_event->jitter / 8;
			bw_bytes_per_second -= latency_bytes;
			bw_bytes = static_cast<uint64_t>(bw_bytes_per_second / 1000);
		} else if (i_event->i_type == model::InboundEvent::device_register) {
			std::shared_ptr<model::ComputationDevice> computationDevice = std::make_shared<model::ComputationDevice>(
				PER_DEVICE_CORES, i_event->source_device);
			network->devices[i_event->source_device] = computationDevice;
		}

		std::chrono::time_point<std::chrono::system_clock> finish = std::chrono::system_clock::now();
		auto temp_time = model::TimeSourceSingleton::getTime();
		auto time = model::TimeSourceSingleton::setTime(temp_time + (finish - start));

		if (return_event != nullptr)
			return_event->event_time = time;
		return return_event;
	}

	std::vector<std::shared_ptr<model::SimEvent> >
	handle_work(const std::shared_ptr<model::SimEvent> &event, std::shared_ptr<model::Network> network,
	            std::map<std::string, std::shared_ptr<model::BaseCompResult> > &off_total,
	            std::map<std::string, std::shared_ptr<model::LowCompResult> > &off_low,
	            std::map<std::string, std::shared_ptr<model::HighCompResult> > &off_high, uint64_t bw_bytes,
	            std::map<std::string, std::tuple<int, int, std::pair<std::chrono::time_point<std::chrono::steady_clock>,std::chrono::time_point<std::chrono::steady_clock>>, enums::dnn_type, int> > &resultMap,
	            std::vector<std::string> state_u_list,
	            bool isLight,
	            int e_queue_size) {
		std::vector<std::shared_ptr<model::SimEvent> > resultVect;
		std::chrono::time_point<std::chrono::system_clock> start = std::chrono::system_clock::now();

		std::map<std::string, std::shared_ptr<model::ComputationDevice> > copyDeviceWorkloadList;

		for (const auto &[key, value]: network->devices) {
			auto temp = std::make_shared<model::ComputationDevice>(value->getCores(), value->getHostName());
			for (const auto &task: value->DNNS)
				temp->DNNS.push_back(task);
			copyDeviceWorkloadList[key] = temp;
		}

		/*Need to create a copy of the network list so that we can keep track of incomplete net allocations */
		std::vector<std::shared_ptr<model::NetCommunicationBase> > copyList;
		for (const auto &i: network->network_link) {
			copyList.push_back(i);
		}

		std::shared_ptr<model::WorkQueueEvent> w_event = std::static_pointer_cast<model::WorkQueueEvent>(event);

		switch (w_event->work_item->getRequestType()) {
			case enums::request_type::low_complexity: {
				std::shared_ptr<model::LowProcessingItem> l_proc = std::static_pointer_cast<model::LowProcessingItem>(
					w_event->work_item);

#ifndef NDEBUG
				std::cout << "Low Complexity: " << l_proc->getDnnIdAndDevice().first << std::endl;
#endif
				std::string sourceHost = l_proc->getDnnIdAndDevice().second;

				if (isLight) {
					auto [o_high, o_low, o_total, rMap, rVect, ntwrk] = services::light_work_function_low_comp(
						bw_bytes, network, sourceHost, w_event, resultVect, event, off_total, off_low, off_high, resultMap, l_proc, e_queue_size);

					off_high = o_high;
					off_low = o_low;
					off_total = o_total;
					resultMap = rMap;
					resultVect = rVect;
					network = ntwrk;
				} else {
					auto [o_high, o_low, o_total, rMap, rVect, ntwrk] = services::work_function_low_comp(
						bw_bytes, network, sourceHost, w_event, resultVect, event, off_total, off_low, off_high, resultMap, e_queue_size);

					off_high = o_high;
					off_low = o_low;
					off_total = o_total;
					resultMap = rMap;
					resultVect = rVect;
					network = ntwrk;
				}
			}
			break;
			case enums::request_type::high_complexity: {
				std::shared_ptr<model::HighProcessingItem> h_proc = std::static_pointer_cast<model::HighProcessingItem>(
					w_event->work_item);

#ifndef NDEBUG
				std::cout << "High Complexity: " << h_proc->getDnnId() << std::endl;
#endif
				std::string sourceHost = (*h_proc->getHostList())[0];

				if (isLight) {
					auto [o_high, o_low, o_total, rMap, rVect, ntwrk, cpyList, cpyMap] = services::light_work_function_high_comp(
						copyDeviceWorkloadList, h_proc, off_total, off_low, off_high, copyList, sourceHost, bw_bytes, state_u_list, network, resultMap,
						resultVect, e_queue_size);

					off_high = o_high;
					off_low = o_low;
					off_total = o_total;
					resultMap = rMap;
					resultVect = rVect;
					network = ntwrk;
					copyList = cpyList;
					copyDeviceWorkloadList = cpyMap;
				} else {
					auto [o_high, o_low, o_total, rMap, rVect, ntwrk, cpyList, cpyMap] = services::work_function_high_comp(
						copyDeviceWorkloadList, h_proc, off_total, off_low, off_high, copyList, sourceHost, bw_bytes, state_u_list, network, resultMap,
						resultVect, e_queue_size);

					off_high = o_high;
					off_low = o_low;
					off_total = o_total;
					resultMap = rMap;
					resultVect = rVect;
					network = ntwrk;
					copyList = cpyList;
					copyDeviceWorkloadList = cpyMap;
				}
			}
			break;
			case enums::request_type::halt_req: {
#ifndef NDEBUG
				std::cout << "Halt Req: ";
#endif
				std::shared_ptr<model::HaltWorkItem> halt_e = std::static_pointer_cast<model::HaltWorkItem>(
					w_event->work_item);

				auto device_of_interest = network->devices[halt_e->getHostToExamine()];

				auto [pruned_dnn, device_task_load, copied_list, total_tasks, total_high, version_to_prune] = halt_call_func(
					halt_e, device_of_interest->DNNS, copyList, off_total, off_high);

				if (pruned_dnn != nullptr) {
					device_of_interest->DNNS = device_task_load;
					network->network_link = copied_list;
					off_total = total_tasks;
					off_high = total_high;

					off_total.erase(pruned_dnn->getDnnId());
					off_high.erase(pruned_dnn->getDnnId());

#ifndef NDEBUG
					std::cout << pruned_dnn->getDnnId();
#endif

					std::shared_ptr<model::HaltNetworkCommsModel> baseNetworkCommsModel = std::make_shared<model::HaltNetworkCommsModel>(
						enums::network_comms_types::halt_req,
						model::TimeSourceSingleton::getTime(), device_of_interest->getHostName(),
						pruned_dnn->getDnnId(),
						version_to_prune);

					auto outbound_event_item = std::make_shared<model::OutboundWorkItem>();
					outbound_event_item->network_work_item = baseNetworkCommsModel;
					outbound_event_item->type = model::SimEvent::Outbound;
					resultVect.push_back(outbound_event_item);
				}
				//If it's light the data-structures must be regenerated
				if (isLight)
					network->devices[halt_e->getHostToExamine()] = services::light_sched_regenerate_res_data_structure(
						device_of_interest->getHostName(), device_of_interest);

			}
			break;
			case enums::request_type::state_update: {
				std::shared_ptr<model::StateUpdate> state_u = std::static_pointer_cast<model::StateUpdate>(
					w_event->work_item);
				std::string dnn_id = state_u->getDnnId();

#ifndef NDEBUG
				std::cout << "State Update: " << dnn_id << std::endl;
#endif

				auto dnn = off_total[dnn_id];

				if (dnn != nullptr) {
					std::string allocated_host = (dnn->getDnnType() == enums::dnn_type::low_comp)
						                             ? dnn->getSrcHost()
						                             : dnn->getAllocatedHost();
					services::state_update_function(off_total, off_low, off_high, dnn_id, off_total[dnn_id],
					                                network->devices[allocated_host], network->network_link,
					                                state_u->isSuccess(), state_u->getFinishTime());
				} else {
					off_total.erase(dnn_id);
				}
			}
			break;
			case enums::request_type::network_disc: {
				if (isLight) {
					auto new_network = services::update_network_disc_prof(bw_bytes, network);
					network = new_network;
					auto time_point = model::TimeSourceSingleton::getTime();
					auto next_disc = time_point + std::chrono::milliseconds{NETWORK_PING_FREQ_MS};

					std::shared_ptr<model::WorkQueueEvent> work_event = std::make_shared<model::WorkQueueEvent>();
					work_event->event_time = next_disc;
					work_event->type = model::SimEvent::Work;
					work_event->work_item = std::make_shared<model::WorkItem>(enums::request_type::network_disc);

					resultVect.push_back(work_event);
				}
			}
			break;
		}

		std::chrono::time_point<std::chrono::system_clock> finish = std::chrono::system_clock::now();

		auto time = model::TimeSourceSingleton::setTime(model::TimeSourceSingleton::getTime() + (finish - start));

		return resultVect;
	}


	std::map<std::string, std::tuple<int, int, std::pair<std::chrono::time_point<std::chrono::steady_clock>,std::chrono::time_point<std::chrono::steady_clock>>, enums::dnn_type, int> >
	profiler_event_loop(std::vector<std::string> state_u_list, std::vector<std::shared_ptr<model::SimEvent> > e_queue, bool isLight) {
		std::shared_ptr<model::Network> network = std::make_shared<model::Network>();
		std::map<std::string, std::shared_ptr<model::BaseCompResult> > off_total;
		std::map<std::string, std::shared_ptr<model::LowCompResult> > off_low;
		std::map<std::string, std::shared_ptr<model::HighCompResult> > off_high;

		std::vector<std::shared_ptr<model::SimEvent> > event_queue(std::move(e_queue));

		std::sort(event_queue.begin(), event_queue.end(),
		          [](const std::shared_ptr<model::SimEvent> &a, const std::shared_ptr<model::SimEvent> &b) {
			          return a->event_time < b->event_time;
		          });

		uint64_t bw_bytes = 0;

		std::map<std::string, std::tuple<int, int, std::pair<std::chrono::time_point<std::chrono::steady_clock>,std::chrono::time_point<std::chrono::steady_clock>>, enums::dnn_type, int> > result_map;

		model::TimeSourceSingleton::setTime(event_queue[0]->event_time - std::chrono::milliseconds{1});

		auto next_disc = event_queue[0]->event_time - std::chrono::milliseconds{1};

		std::shared_ptr<model::WorkQueueEvent> work_event = std::make_shared<model::WorkQueueEvent>();
		work_event->event_time = next_disc;
		work_event->type = model::SimEvent::Work;
		work_event->work_item = std::make_shared<model::WorkItem>(enums::request_type::network_disc);
		event_queue.insert(event_queue.begin(), work_event);


		bool new_e = false;
#ifndef NDEBUG
		int counter = 0;
		int inbound_counter = 0;

#endif
		while (!event_queue.empty()) {
			std::shared_ptr<model::SimEvent> event = event_queue[0];
			event_queue.erase(event_queue.begin());

			// if (event->event_time > model::TimeSourceSingleton::getTime())
			model::TimeSourceSingleton::setTime(event->event_time);

			switch (event->type) {
				case model::SimEvent::Inbound: {
					auto result = handle_inbound(event, bw_bytes, network, result_map, event_queue.size());


#ifndef NDEBUG
					inbound_counter++;
#endif

					if (result != nullptr) {
						new_e = true;
						event_queue.push_back(result);
					}
				}
				break;

				case model::SimEvent::Work: {
#ifndef NDEBUG
					counter++;
					std::cout << "Work: W" << counter << " - I" << inbound_counter << std::endl;
#endif
					auto results = handle_work(event, network, off_total, off_low, off_high, bw_bytes, result_map, state_u_list, isLight, static_cast<int>(event_queue.size()));
					for (const auto result: results) {
						if (result != nullptr) {
							auto work_event = std::static_pointer_cast<model::WorkQueueEvent>(result);
							if (!(event_queue.empty() && work_event->work_item->getRequestType() == enums::request_type::network_disc && results.size() == 1))
								event_queue.push_back(result);
						}
					}
					break;
				}
			}

			std::sort(event_queue.begin(), event_queue.end(),
			          [](std::shared_ptr<model::SimEvent> a, std::shared_ptr<model::SimEvent> b) {
				          return a->event_time < b->event_time;
			          });
		}

		return result_map;
	} // services

	std::string write_profile(web::json::value log, std::string output_path) {
		std::cout << "Simulation Complete - Writing results to " << output_path << std::endl;

		auto serialised_result = log.serialize();
		std::ofstream file(output_path);
		if (file.is_open()) {
			file << log.serialize();
			file.close();
		}

		return serialised_result;
	}

	std::map<std::string, std::tuple<int, int, std::shared_ptr<model::TimeWindow>, enums::dnn_type> >
	printResults(std::map<std::string, std::tuple<int, int, std::shared_ptr<model::TimeWindow>, enums::dnn_type> > resultMap) {
		std::vector<web::json::value> logList;

		for (const auto &[dnn_id, res_tuple]: resultMap) {
			auto [starting_task_count, finish_task_count, tw, dnnType] = res_tuple;
			auto log = web::json::value::object();
			log["dnn_id"] = web::json::value::string(dnn_id);
			log["starting_task_count"] = web::json::value::number(starting_task_count);
			log["finish_task_count"] = web::json::value::number(finish_task_count);
			log["time_window"] = tw->convertToJson();
			logList.push_back(log);
		}

		auto arr = web::json::value::array(logList);
		write_profile(arr, PROFILE_RESULT_FILE);

		return resultMap;
	}

	void writeTransformedLoadResults(
		std::pair<std::map<int, std::pair<float, std::vector<std::tuple<uint64_t, uint64_t, int, std::string>> > >, std::map<int, std::pair<float, std::vector<std::tuple<uint64_t, uint64_t, int, std::string>> > >> load_transformation, std::string output_path) {
		web::json::value load_map;

		web::json::value high_map;
		web::json::value low_map;

		for (const auto &[load_key, map_values]: load_transformation.first) {
			web::json::value local_map;

			auto [avg, latency_list] = map_values;

			std::vector<web::json::value> latency_json_array;

			for (const auto &[tw_start, tw_finish, event_queue_size, dnn_id]: latency_list) {
				auto temp_obj = web::json::value::object();

				temp_obj["tw_start"] = web::json::value::number(tw_start);
				temp_obj["tw_finish"] = web::json::value::number(tw_finish);
				temp_obj["event_queue_size"] = web::json::value::number(event_queue_size);
				latency_json_array.push_back(temp_obj);
			}

			local_map["average"] = web::json::value::number(avg);
			local_map["latency_values"] = web::json::value::array(latency_json_array);
			low_map[std::to_string(load_key)] = local_map;
		}

		for (const auto &[load_key, map_values]: load_transformation.second) {
			web::json::value local_map;

			auto [avg, latency_list] = map_values;

			std::vector<web::json::value> latency_json_array;

			for (const auto &[tw_start, tw_finish, event_queue_size, dnn_id]: latency_list) {
				auto temp_obj = web::json::value::object();
				temp_obj["tw_start"] = web::json::value::number(tw_start);
				temp_obj["tw_finish"] = web::json::value::number(tw_finish);
				temp_obj["event_queue_size"] = web::json::value::number(event_queue_size);
				latency_json_array.push_back(temp_obj);
			}

			local_map["average"] = web::json::value::number(avg);
			local_map["latency_values"] = web::json::value::array(latency_json_array);

			high_map[std::to_string(load_key)] = local_map;
		}

		load_map["high"] = high_map;
		load_map["low"] = low_map;

		write_profile(load_map, output_path);
	}

	std::pair<std::map<int, std::pair<float, std::vector<std::tuple<uint64_t, uint64_t, int, std::string>> > >, std::map<int, std::pair<float, std::vector<std::tuple<uint64_t, uint64_t, int, std::string>> > >>
	profile_data_transform(
		std::map<std::string, std::tuple<int, int, std::pair<std::chrono::time_point<std::chrono::steady_clock>,std::chrono::time_point<std::chrono::steady_clock>>, enums::dnn_type, int> > profile_map) {
		std::map<int, std::pair<float, std::vector<std::tuple<uint64_t, uint64_t, int, std::string>> > > return_obj_high;
		std::map<int, std::pair<float, std::vector<std::tuple<uint64_t, uint64_t, int, std::string>> > > return_obj_low;


		for (const auto &[task_result_key, task_result_value]: profile_map) {
			const auto [task_ingress, task_egress, tw, dnnType, event_queue_size] = task_result_value;
			auto key = std::max(task_ingress, task_egress);

			std::map<int, std::pair<float, std::vector<std::tuple<uint64_t, uint64_t, int, std::string>>> > &obj_to_peruse = (dnnType == enums::dnn_type::high_comp)
				                                                                          ? return_obj_high
				                                                                          : return_obj_low;
			if (!obj_to_peruse.contains(key)) {
				obj_to_peruse[key] = std::pair(0, std::vector<std::tuple<uint64_t, uint64_t, int, std::string>>());
			}

			auto tw_start_conversion_ms = std::chrono::duration_cast<std::chrono::milliseconds>(tw.first.time_since_epoch()).count();
			auto tw_finish_conversion_ms = std::chrono::duration_cast<std::chrono::milliseconds>(tw.first.time_since_epoch()).count();

			obj_to_peruse[key].second.emplace_back(
				tw_start_conversion_ms, tw_finish_conversion_ms, event_queue_size, task_result_key);
		}

		for (auto &[key, value]: return_obj_high) {
			auto &[avg, vec] = value;

			if (!vec.empty()) {
				uint64_t sum = 0;

				for (const auto& [tw_start, tw_stop, e_q, dnn_id]: vec) {
					sum += tw_stop - tw_start;
				}

				avg = static_cast<float>(sum) / vec.size();
			} else {
				avg = 0.0f;
			}
		}

		for (auto &[key, value]: return_obj_low) {
			auto &[avg, vec] = value;

			if (!vec.empty()) {
				uint64_t sum = 0;
				for (const auto& [tw_start, tw_stop, e_q, dnn_id]: vec)
					sum += tw_stop - tw_start;
				avg = static_cast<float>(sum) / vec.size();
			} else {
				avg = 0.0f;
			}
		}

		return std::make_pair(return_obj_low, return_obj_high);
	}
}
