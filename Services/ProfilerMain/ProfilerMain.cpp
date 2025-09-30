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
#include "../../Constants/LOG_CONSTANT.h"

namespace services {
	std::shared_ptr<model::SimEvent>
	handle_inbound(const std::shared_ptr<model::SimEvent> &event, uint64_t &bw_bytes, std::shared_ptr<model::Network> network,
				   std::map<std::string, std::tuple<int, int, std::shared_ptr<model::TimeWindow>, enums::dnn_type>> &resultMap) {
		std::chrono::time_point<std::chrono::system_clock> start = std::chrono::system_clock::now();

		std::shared_ptr<model::InboundEvent> i_event = std::static_pointer_cast<model::InboundEvent>(event);
		std::shared_ptr<model::SimEvent> return_event;
		if (i_event->i_type == model::InboundEvent::high_offload) {
			std::shared_ptr<model::HighOffload> h_event = std::static_pointer_cast<model::HighOffload>(i_event);
			std::vector<std::string> dnnIds;

			dnnIds.reserve(h_event->task_count);

			int tasks_in_network = numberOfTasksInNetwork(network);
			for (int i = 0; i < h_event->task_count; i++) {
				auto dnn_id = h_event->dnn_id + "_" + std::to_string(i);
				dnnIds.push_back(dnn_id);


				if (resultMap.find(dnn_id) == resultMap.end()) {
					auto tw = std::make_shared<model::TimeWindow>(std::chrono::system_clock::now(),
																  std::chrono::system_clock::now());
					resultMap[dnn_id] = std::make_tuple(tasks_in_network, -1, tw, enums::dnn_type::high_comp);
				} else {
					auto [beginningTaskCount, finTaskCount, window, dnnType] = resultMap[dnn_id];
					window->start = std::chrono::system_clock::now();
					resultMap[dnn_id] = std::make_tuple(tasks_in_network, finTaskCount, window, dnnType);
				}
			}


			auto hostList = std::make_shared<std::vector<std::string>>(
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
			std::shared_ptr<std::vector<std::string>> hostList = std::make_shared<std::vector<std::string>>(
					std::initializer_list{i_event->source_device});
			std::shared_ptr<model::LowProcessingItem> l_proc = std::make_shared<model::LowProcessingItem>(hostList,
																										  enums::request_type::low_complexity,
																										  i_event->d_time,
																										  dnnIdsAndDevice);

			int tasks_in_network = numberOfTasksInNetwork(network);
			if (resultMap.find(dnn_id) == resultMap.end()) {
				auto tw = std::make_shared<model::TimeWindow>(std::chrono::system_clock::now(), std::chrono::system_clock::now());
				resultMap[dnn_id] = std::make_tuple(tasks_in_network, -1, tw, enums::dnn_type::low_comp);
			} else {
				auto [beginningTaskCount, finTaskCount, window, dnnType] = resultMap[dnn_id];
				window->start = std::chrono::system_clock::now();
				resultMap[dnn_id] = std::make_tuple(tasks_in_network, finTaskCount, window, dnnType);
			}

			auto res = std::make_shared<model::WorkQueueEvent>();
			res->work_item = l_proc;
			res->type = model::SimEvent::Work;
			return_event = std::static_pointer_cast<model::SimEvent>(res);
		} else if (i_event->i_type == model::InboundEvent::state_update || i_event->i_type == model::InboundEvent::deadline_violation) {
			std::string dnn_id = i_event->dnn_id;

			auto hostList = std::make_shared<std::vector<std::string>>(
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
			auto temp_event = std::make_shared<model::IperfResult>();

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

	std::vector<std::shared_ptr<model::SimEvent>>
	handle_work(const std::shared_ptr<model::SimEvent> &event, std::shared_ptr<model::Network> network,
				std::map<std::string, std::shared_ptr<model::BaseCompResult>> &off_total,
				std::map<std::string, std::shared_ptr<model::LowCompResult>> &off_low,
				std::map<std::string, std::shared_ptr<model::HighCompResult>> &off_high, uint64_t bw_bytes,
				std::map<std::string, std::tuple<int, int, std::shared_ptr<model::TimeWindow>, enums::dnn_type>> &resultMap,
				std::vector<std::string> state_u_list) {
		std::vector<std::shared_ptr<model::SimEvent>> resultVect;
		std::chrono::time_point<std::chrono::system_clock> start = std::chrono::system_clock::now();

		std::map<std::string, std::shared_ptr<model::ComputationDevice>> copyDeviceWorkloadList;

		for (const auto &[key, value]: network->devices) {
			auto temp = std::make_shared<model::ComputationDevice>(value->getCores(), value->getHostName());
			for (const auto &task: value->DNNS)
				temp->DNNS.push_back(task);
			copyDeviceWorkloadList[key] = temp;
		}

		/*Need to create a copy of the network list so that we can keep track of incomplete net allocations */
		std::vector<std::shared_ptr<model::NetCommunicationBase>> copyList;
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
				auto [task, time_window] = services::low_comp_allocation_func(bw_bytes, network, sourceHost,
																			  w_event->work_item);

				//Construct preemption request
				if (task == nullptr) {
					auto preemption_request = std::make_shared<model::HaltWorkItem>(enums::request_type::halt_req,
																					sourceHost, time_window->start,
																					time_window->stop);

					auto preemption_work_event = std::make_shared<model::WorkQueueEvent>();
					preemption_work_event->type = model::SimEvent::Work;
					preemption_work_event->work_item = preemption_request;
					resultVect.push_back(preemption_work_event);
					resultVect.push_back(event);
				} else {
					network->devices[sourceHost]->DNNS.push_back(task);
					off_total[task->getDnnId()] = task;
					off_low[task->getDnnId()] = task;
					auto outbound_low_comp_comms = std::make_shared<model::LowComplexityAllocationComms>(
							enums::network_comms_types::low_complexity_allocation,
							model::TimeSourceSingleton::getTime() + std::chrono::milliseconds{1}, task, sourceHost);

					auto outbound_event_item = std::make_shared<model::OutboundWorkItem>();
					outbound_event_item->network_work_item = outbound_low_comp_comms;
					outbound_event_item->type = model::SimEvent::Outbound;
					resultVect.push_back(outbound_event_item);
				}

				int tasks_in_network = numberOfTasksInNetwork(network);
				if (task != nullptr) {
					if (resultMap.find(task->getDnnId()) == resultMap.end()) {
						auto tw = std::make_shared<model::TimeWindow>(std::chrono::system_clock::now(),
																	  std::chrono::system_clock::now());
						resultMap[task->getDnnId()] = std::make_tuple(-1, tasks_in_network, tw, enums::dnn_type::low_comp);
					} else {
						auto [beginningTaskCount, finTaskCount, window, dnnType] = resultMap[task->getDnnId()];
						window->stop = std::chrono::system_clock::now();
						resultMap[task->getDnnId()] = std::make_tuple(beginningTaskCount, tasks_in_network, window, dnnType);
					}
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

				auto result = services::high_comp_allocation_func(h_proc->isReallocation(), h_proc, off_total,
																  copyDeviceWorkloadList,
																  copyList, bw_bytes, sourceHost);

//				We have to account for the scenario where a task is allocated that was not successfully allocated in the original experiment
				for (const auto &task: result) {
					if (std::find(state_u_list.begin(), state_u_list.end(), task->getDnnId()) == state_u_list.end()) {
						std::string dnn_id = task->getDnnId();

						auto hostList = std::make_shared<std::vector<std::string>>(
								std::initializer_list<std::string>{task->getAllocatedHost()});

						std::shared_ptr<model::StateUpdate> s_update = std::make_shared<model::StateUpdate>(hostList,
																											enums::request_type::state_update,
																											task->estimated_start_fin->stop + std::chrono::milliseconds{
																													1},
																											dnn_id);
						s_update->setSuccess(true);

						auto res = std::make_shared<model::WorkQueueEvent>();
						res->work_item = s_update;
						res->type = model::SimEvent::Work;
						resultVect.push_back(res);
					}
					network->devices[task->getAllocatedHost()]->DNNS.push_back(task);
					if (sourceHost != task->getAllocatedHost())
						network->addComm(task->task_allocation);

					off_total[task->getDnnId()] = task;
					off_high[task->getDnnId()] = task;

					std::shared_ptr<model::HighComplexityAllocationComms> high_comp_comms = std::make_shared<model::HighComplexityAllocationComms>(
							enums::network_comms_types::high_complexity_task_mapping,
							task->estimated_start_fin->start,
							task,
							task->getSrcHost());

					auto outbound_event_item = std::make_shared<model::OutboundWorkItem>();
					outbound_event_item->network_work_item = high_comp_comms;
					outbound_event_item->type = model::SimEvent::Outbound;
					resultVect.push_back(outbound_event_item);

				}

				for (const auto &task: result) {

					int tasks_in_network = numberOfTasksInNetwork(network);
					if (resultMap.find(task->getDnnId()) == resultMap.end()) {
						auto tw = std::make_shared<model::TimeWindow>(std::chrono::system_clock::now(),
																	  std::chrono::system_clock::now());
						resultMap[task->getDnnId()] = std::make_tuple(-1, tasks_in_network, tw, enums::dnn_type::high_comp);
					} else {
						auto [beginningTaskCount, finTaskCount, window, dnnType] = resultMap[task->getDnnId()];
						window->stop = std::chrono::system_clock::now();
						resultMap[task->getDnnId()] = std::make_tuple(beginningTaskCount, tasks_in_network, window, dnnType);
					}
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

				device_of_interest->DNNS = device_task_load;
				network->network_link = copied_list;
				off_total = total_tasks;
				off_high = total_high;

				off_total.erase(pruned_dnn->getDnnId());
				off_high.erase(pruned_dnn->getDnnId());

				std::shared_ptr<model::HaltNetworkCommsModel> baseNetworkCommsModel = std::make_shared<model::HaltNetworkCommsModel>(
						enums::network_comms_types::halt_req,
						model::TimeSourceSingleton::getTime(), device_of_interest->getHostName(),
						pruned_dnn->getDnnId(),
						version_to_prune);
#ifndef NDEBUG
				std::cout << pruned_dnn->getDnnId() << std::endl;
#endif
				auto outbound_event_item = std::make_shared<model::OutboundWorkItem>();
				outbound_event_item->network_work_item = baseNetworkCommsModel;
				outbound_event_item->type = model::SimEvent::Outbound;
				resultVect.push_back(outbound_event_item);
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
												 ? dnn->getSrcHost() : dnn->getAllocatedHost();
					services::state_update_function(off_total, off_low, off_high, dnn_id, off_total[dnn_id],
													network->devices[allocated_host], network->network_link,
													state_u->isSuccess(), state_u->getFinishTime());
				} else {
					off_total.erase(dnn_id);
				}
			}
				break;
		}

		std::chrono::time_point<std::chrono::system_clock> finish = std::chrono::system_clock::now();

		auto time = model::TimeSourceSingleton::setTime(model::TimeSourceSingleton::getTime() + (finish - start));

		return resultVect;
	}


	std::map<std::string, std::tuple<int, int, std::shared_ptr<model::TimeWindow>, enums::dnn_type>>
	profiler_event_loop(std::vector<std::string> state_u_list, std::vector<std::shared_ptr<model::SimEvent>> e_queue) {
		std::shared_ptr<model::Network> network = std::make_shared<model::Network>();
		std::map<std::string, std::shared_ptr<model::BaseCompResult>> off_total;
		std::map<std::string, std::shared_ptr<model::LowCompResult>> off_low;
		std::map<std::string, std::shared_ptr<model::HighCompResult>> off_high;

		std::vector<std::shared_ptr<model::SimEvent>> event_queue(std::move(e_queue));

		std::sort(event_queue.begin(), event_queue.end(),
				  [](const std::shared_ptr<model::SimEvent> &a, const std::shared_ptr<model::SimEvent> &b) {
					  return a->event_time < b->event_time;
				  });

		uint64_t bw_bytes = 0;

		std::map<std::string, std::tuple<int, int, std::shared_ptr<model::TimeWindow>, enums::dnn_type>> result_map;

		model::TimeSourceSingleton::setTime(event_queue[0]->event_time);
#ifndef NDEBUG
		int counter = 0;
		int inbound_counter = 0;
#endif
		while (!event_queue.empty()) {
			std::shared_ptr<model::SimEvent> event = event_queue[0];
			event_queue.erase(event_queue.begin());

			if (event->event_time > model::TimeSourceSingleton::getTime())
				model::TimeSourceSingleton::setTime(event->event_time);

			switch (event->type) {
				case model::SimEvent::Inbound: {
					auto result = handle_inbound(event, bw_bytes, network, result_map);

#ifndef NDEBUG
					inbound_counter++;
#endif

					if (result != nullptr)
						event_queue.push_back(result);
				}
					break;

				case model::SimEvent::Work: {
#ifndef NDEBUG
					counter++;
					std::cout << "Work: W" << counter << " - I" << inbound_counter << std::endl;
#endif
					auto results = handle_work(event, network, off_total, off_low, off_high, bw_bytes, result_map, state_u_list);
					for (const auto result: results) {
						if (result != nullptr)
							event_queue.push_back(result);
					}
					break;
				}
			}

			std::sort(event_queue.begin(), event_queue.end(),
					  [](std::shared_ptr<model::SimEvent> a, std::shared_ptr<model::SimEvent> b) {
						  return a->event_time < b->event_time;
					  });
		}

//		return printResults(result_map);
		return result_map;
	} // services

	std::string write_profile(web::json::value log) {
		auto serialised_result = log.serialize();
		std::ofstream file(PROFILE_RESULT_FILE);
		if (file.is_open()) {
			file << log.serialize();
			file.close();
		}

		return serialised_result;
	}

	std::map<std::string, std::tuple<int, int, std::shared_ptr<model::TimeWindow>, enums::dnn_type>>
	printResults(std::map<std::string, std::tuple<int, int, std::shared_ptr<model::TimeWindow>, enums::dnn_type>> resultMap) {
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
		write_profile(arr);

		return resultMap;
	}


	int numberOfTasksInNetwork(std::shared_ptr<model::Network> network) {
		int tasks = 0;
		for (const auto &[hostname, device]: network->devices) {
			tasks += static_cast<int>(device->DNNS.size());
		}

		return tasks;
	}

	void writeTransformedLoadResults(
			std::pair<std::map<int, std::pair<float, std::vector<uint64_t>>>, std::map<int, std::pair<float, std::vector<uint64_t>>>> load_transformation) {

		web::json::value load_map;

		web::json::value high_map;
		web::json::value low_map;

		for (const auto &[load_key, map_values]: load_transformation.first) {
			web::json::value local_map;

			auto [avg, latency_list] = map_values;

			std::vector<web::json::value> latency_json_array;

			for (const auto &latency_value: latency_list)
				latency_json_array.push_back(web::json::value::number(latency_value));

			local_map["average"] = web::json::value::number(avg);
			local_map["latency_values"] = web::json::value::array(latency_json_array);
			low_map[std::to_string(load_key)] = local_map;
		}

		for (const auto &[load_key, map_values]: load_transformation.second) {
			web::json::value local_map;

			auto [avg, latency_list] = map_values;

			std::vector<web::json::value> latency_json_array;

			for (const auto &latency_value: latency_list)
				latency_json_array.push_back(web::json::value::number(latency_value));

			local_map["average"] = web::json::value::number(avg);
			local_map["latency_values"] = web::json::value::array(latency_json_array);

			high_map[std::to_string(load_key)] = local_map;
		}

		load_map["high"] = high_map;
		load_map["low"] = low_map;

		write_profile(load_map);
	}

	std::pair<std::map<int, std::pair<float, std::vector<uint64_t>>>, std::map<int, std::pair<float, std::vector<uint64_t>>>>
	profile_data_transform(
			std::map<std::string, std::tuple<int, int, std::shared_ptr<model::TimeWindow>, enums::dnn_type>> profile_map) {
		std::map<int, std::pair<float, std::vector<uint64_t>>> return_obj_high;
		std::map<int, std::pair<float, std::vector<uint64_t>>> return_obj_low;
		for (const auto &[task_result_key, task_result_value]: profile_map) {
			const auto [task_ingress, task_egress, tw, dnnType] = task_result_value;
			auto key = std::max(task_ingress, task_egress);

			std::map<int, std::pair<float, std::vector<uint64_t>>> &obj_to_peruse = (dnnType == enums::dnn_type::high_comp)
																					? return_obj_high : return_obj_low;
			if (!obj_to_peruse.contains(key)) {
				obj_to_peruse[key] = std::make_pair(0, std::vector<uint64_t>());
			}
			obj_to_peruse[key].second.push_back(
					std::chrono::duration_cast<std::chrono::milliseconds>(tw->stop - tw->start).count());

		}

		for (auto &[key, value]: return_obj_high) {
			auto &[avg, vec] = value;

			if (!vec.empty()) {
				uint64_t sum = std::accumulate(vec.begin(), vec.end(), uint64_t{0});
				avg = static_cast<float>(sum) / vec.size();
			} else {
				avg = 0.0f;
			}
		}

		for (auto &[key, value]: return_obj_low) {
			auto &[avg, vec] = value;

			if (!vec.empty()) {
				uint64_t sum = std::accumulate(vec.begin(), vec.end(), uint64_t{0});
				avg = static_cast<float>(sum) / vec.size();
			} else {
				avg = 0.0f;
			}
		}

		return std::make_pair(return_obj_low, return_obj_high);
	}
}