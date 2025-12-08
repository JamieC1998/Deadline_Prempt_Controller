//
// Created by Jamie Cotter on 21/10/2025.
//

#include "HeavyProfilerFunctions.h"
#include "../../../model/data_models/WorkItems/StateUpdate/StateUpdate.h"
#include "../../../model/data_models/NetworkCommsModels/HighComplexityAllocation/HighComplexityAllocationComms.h"

namespace services {
	std::tuple<std::map<std::string, std::shared_ptr<model::HighCompResult> >, std::map<std::string, std::shared_ptr<model::LowCompResult> >, std::map<
			std::string, std::shared_ptr<model::BaseCompResult> >, std::map<std::string, std::tuple<int, int, std::pair<std::chrono::time_point<std::chrono::steady_clock>,std::chrono::time_point<std::chrono::steady_clock>>, enums::dnn_type, int> >
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
	                        std::map<std::string, std::tuple<int, int, std::pair<std::chrono::time_point<std::chrono::steady_clock>,std::chrono::time_point<std::chrono::steady_clock>>, enums::dnn_type, int> > resultMap,
	                        std::vector<std::shared_ptr<model::SimEvent> > resultVect,
	                        int e_queue_size) {

		auto latency_start_time = std::chrono::steady_clock::now();
		auto result = services::high_comp_allocation_func(h_proc->isReallocation(), h_proc, off_total,
		                                                  copyDeviceWorkloadList,
		                                                  copyList, bw_bytes, sourceHost);

		std::vector<std::chrono::time_point<std::chrono::steady_clock>> latency_end_times;

		for (const auto &result: result)
			latency_end_times.push_back(std::chrono::steady_clock::now());

		// We have to account for the scenario where a task is allocated that was not successfully allocated in the original experiment
		for (const auto &task: result) {
			if (std::find(state_u_list.begin(), state_u_list.end(), task->getDnnId()) == state_u_list.end()) {
				std::string dnn_id = task->getDnnId();

				auto hostList = std::make_shared<std::vector<std::string> >(
					std::initializer_list<std::string>{task->getAllocatedHost()});

				std::shared_ptr<model::StateUpdate> s_update = std::make_shared<model::StateUpdate>(hostList,
				                                                                                    enums::request_type::state_update,
				                                                                                    task->estimated_start_fin->stop + std::chrono::milliseconds{
					                                                                                    1
				                                                                                    },
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

		for (int i = 0; i < result.size(); i++) {
			auto task = result[i];
			int tasks_in_network = utils::numberOfTasksInNetwork(network);

			if (resultMap.find(task->getDnnId()) == resultMap.end()) {
				auto tw = std::make_pair(std::chrono::steady_clock::now(),
				                                              std::chrono::steady_clock::now());
				resultMap[task->getDnnId()] = std::make_tuple(-1, tasks_in_network, tw, enums::dnn_type::high_comp, e_queue_size);
			} else {
				auto [beginningTaskCount, finTaskCount, window, dnnType, e_q] = resultMap[task->getDnnId()];
				window.first = latency_start_time;
				window.second = latency_end_times[i];
				resultMap[task->getDnnId()] = std::make_tuple(beginningTaskCount, tasks_in_network, window, dnnType, e_queue_size);
			}
		}
		return std::make_tuple(off_high, off_low, off_total, resultMap, resultVect, network, copyList, copyDeviceWorkloadList);
	}

	std::tuple<std::map<std::string, std::shared_ptr<model::HighCompResult> >, std::map<std::string, std::shared_ptr<model::LowCompResult> >, std::map<
			std::string, std::shared_ptr<model::BaseCompResult> >, std::map<std::string, std::tuple<int, int, std::pair<std::chrono::time_point<std::chrono::steady_clock>,std::chrono::time_point<std::chrono::steady_clock>>, enums::dnn_type, int> >
		, std::vector<std::shared_ptr<model::SimEvent> >, std::shared_ptr<model::Network> >
	work_function_low_comp(uint64_t bw_bytes, std::shared_ptr<model::Network> network, std::string sourceHost,
	                       std::shared_ptr<model::WorkQueueEvent> w_event,
	                       std::vector<std::shared_ptr<model::SimEvent> > resultVect, std::shared_ptr<model::SimEvent> event,
	                       std::map<std::string, std::shared_ptr<model::BaseCompResult> > off_total,
	                       std::map<std::string, std::shared_ptr<model::LowCompResult> > off_low,
	                       std::map<std::string, std::shared_ptr<model::HighCompResult> > off_high,
	                       std::map<std::string, std::tuple<int, int, std::pair<std::chrono::time_point<std::chrono::steady_clock>,std::chrono::time_point<std::chrono::steady_clock>>, enums::dnn_type, int> > resultMap,
	                       int e_queue_size) {
		auto latency_start_time = std::chrono::steady_clock::now();
		auto [task, time_window] = services::low_comp_allocation_func(bw_bytes, network, sourceHost,
		                                                              w_event->work_item);

		auto latency_measure_end = std::chrono::steady_clock::now();
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
		int tasks_in_network = utils::numberOfTasksInNetwork(network);
		if (task != nullptr) {
			if (resultMap.find(task->getDnnId()) == resultMap.end()) {
				auto tw = std::make_pair(std::chrono::steady_clock::now(),
				                                              std::chrono::steady_clock::now());
				resultMap[task->getDnnId()] = std::make_tuple(-1, tasks_in_network, tw, enums::dnn_type::low_comp, e_queue_size);
			} else {
				auto [beginningTaskCount, finTaskCount, window, dnnType, e_q] = resultMap[task->getDnnId()];
				window.first = latency_start_time;
				window.second = latency_measure_end;
				resultMap[task->getDnnId()] = std::make_tuple(beginningTaskCount, tasks_in_network, window, dnnType, e_queue_size);
			}
		}

		return std::make_tuple(off_high, off_low, off_total, resultMap, resultVect, network);
	}
} // services
