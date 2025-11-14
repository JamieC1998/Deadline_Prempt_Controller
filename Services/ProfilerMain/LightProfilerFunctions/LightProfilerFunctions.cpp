//
// Created by Jamie Cotter on 23/10/2025.
//

#include "LightProfilerFunctions.h"

namespace services {
	std::shared_ptr<model::Network>
	update_network_disc_prof(uint64_t bps, std::shared_ptr<model::Network> network) {
		auto disc_link = network->disc_network_link;

		auto [result_link, last_time_of_reasoning] = services::light_sched_update_network_disc(bps, disc_link);

		network->disc_network_link = result_link;
		network->last_time_of_reasoning = last_time_of_reasoning;

		return network;
	}

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
	                             std::shared_ptr<model::LowProcessingItem> l_proc) {
		auto target_device = network->devices[sourceHost];
		auto [result_state, allocated_time_window, resulting_task] = services::light_sched_low_comp_allocation_call(
			l_proc->getDnnIdAndDevice().first, sourceHost, l_proc->getDeadline(), network->devices,
			target_device, l_proc->isReallocation(), bw_bytes);

		if (result_state == ResultState::success || result_state == ResultState::post_preemp_succ) {
			auto bR = resulting_task;

			off_low[bR->getDnnId()] = bR;
			off_total[bR->getDnnId()] = bR;

			std::shared_ptr<model::LowComplexityAllocationComms> baseNetworkCommsModel = std::make_shared<model::LowComplexityAllocationComms>(
				enums::network_comms_types::low_complexity_allocation, bR->estimated_start_fin->start, bR,
				bR->getSrcHost());

			auto outbound_event_item = std::make_shared<model::OutboundWorkItem>();
			outbound_event_item->network_work_item = baseNetworkCommsModel;
			outbound_event_item->type = model::SimEvent::Outbound;
			resultVect.push_back(outbound_event_item);

			int tasks_in_network = utils::numberOfTasksInNetwork(network);

			if (resultMap.find(bR->getDnnId()) == resultMap.end()) {
				auto tw = std::make_shared<model::TimeWindow>(std::chrono::system_clock::now(),
				                                              std::chrono::system_clock::now());
				resultMap[bR->getDnnId()] = std::make_tuple(-1, tasks_in_network, tw, enums::dnn_type::low_comp);
			} else {
				auto [beginningTaskCount, finTaskCount, window, dnnType] = resultMap[bR->getDnnId()];
				window->stop = std::chrono::system_clock::now();
				resultMap[bR->getDnnId()] = std::make_tuple(beginningTaskCount, tasks_in_network, window, dnnType);
			}
		} else {
			auto preemption_request = std::make_shared<model::HaltWorkItem>(enums::request_type::halt_req,
			                                                                sourceHost, allocated_time_window->start,
			                                                                allocated_time_window->stop);

			auto preemption_work_event = std::make_shared<model::WorkQueueEvent>();
			preemption_work_event->type = model::SimEvent::Work;
			preemption_work_event->work_item = preemption_request;
			preemption_work_event->event_time = model::TimeSourceSingleton::getTime();
			resultVect.push_back(preemption_work_event);

			auto newTask = std::make_shared<model::LowProcessingItem>(l_proc->getHostList(),
			                                                          enums::request_type::low_complexity,
			                                                          l_proc->getDeadline(),
			                                                          l_proc->getDnnIdAndDevice());
			newTask->setReallocation(true);
			auto reallocatingTask = std::make_shared<model::WorkQueueEvent>();
			reallocatingTask->type = model::SimEvent::Work;
			reallocatingTask->work_item = newTask;
			reallocatingTask->event_time = model::TimeSourceSingleton::getTime() + std::chrono::milliseconds{1};
			resultVect.push_back(reallocatingTask);
		}

		return std::make_tuple(off_high, off_low, off_total, resultMap, resultVect, network);
	}

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
	                              std::vector<std::shared_ptr<model::SimEvent> > resultVect) {
		std::vector<std::tuple<ResultState, std::string, std::shared_ptr<model::HighCompResult> > > result = services::light_sched_high_comp_allocation_call(
			h_proc->isReallocation(),
			h_proc->getDnnIds(),
			static_cast<double>(bw_bytes),
			std::move(sourceHost),
			h_proc->getDnnId(),
			network->disc_network_link,
			h_proc->getDeadline(),
			network->last_time_of_reasoning,
			network->devices);

		std::vector<std::shared_ptr<model::HighCompResult> > resultItems;

		for (const auto &[result_state, dnn_id_result, result_obj]: result) {
			if (result_state == ResultState::success || result_state == ResultState::post_preemp_succ) {
				resultItems.emplace_back(result_obj);
				if (std::find(state_u_list.begin(), state_u_list.end(), result_obj->getDnnId()) == state_u_list.end()) {
					resultItems.emplace_back(result_obj);

					std::shared_ptr<model::StateUpdate> s_update = std::make_shared<model::StateUpdate>(h_proc->getHostList(),
					                                                                                    enums::request_type::state_update,
					                                                                                    result_obj->estimated_start_fin->stop +
					                                                                                    std::chrono::milliseconds{
						                                                                                    1
					                                                                                    },
					                                                                                    dnn_id_result);
					s_update->setSuccess(true);

					auto res = std::make_shared<model::WorkQueueEvent>();
					res->work_item = s_update;
					res->type = model::SimEvent::Work;
					resultVect.push_back(res);
				}
			}
		}

		for (const auto &task: resultItems) {
			std::shared_ptr<model::HighComplexityAllocationComms> high_comp_comms = std::make_shared<model::HighComplexityAllocationComms>(
				enums::network_comms_types::high_complexity_task_mapping, task->estimated_start_fin->start, task,
				task->getSrcHost());

			std::shared_ptr<model::BaseNetworkCommsModel> comm_task = std::static_pointer_cast<model::BaseNetworkCommsModel>(
				high_comp_comms);
		}
		for (const auto &task: resultItems) {
			if (task->getAllocatedHost() != task->getSrcHost()) {
				/* Marking bucket as occupied */
				auto comm = std::static_pointer_cast<model::Bucket>(task->task_allocation);

				comm->bucketContents.push_back(task->getDnnId());
			}

			off_total[task->getDnnId()] = task;
			off_high[task->getDnnId()] = task;

			auto device = network->devices[task->getAllocatedHost()];
			device->DNNS.push_back(task);
			device->resAvailRemoveAndSplit(task->estimated_start_fin, task->getCoreAllocation(), 0);
		}

		for (const auto &task: resultItems) {
			int tasks_in_network = utils::numberOfTasksInNetwork(network);
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
		return std::make_tuple(off_high, off_low, off_total, resultMap, resultVect, network, copyList, copyDeviceWorkloadList);
	}

	void light_work_state_update() {
	}
} // services
