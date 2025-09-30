//
// Created by Jamie Cotter on 30/07/2025.
//

#include "WorkQueueFunctions.h"
#include "../../model/data_models/WorkItems/StateUpdate/StateUpdate.h"
#include "../../Constants/ModeMacros.h"
#include "../../model/data_models/NetworkCommsModels/HighComplexityAllocation/HighComplexityAllocationComms.h"
#include "../../model/data_models/NetworkCommsModels/HaltNetworkCommsModel/HaltNetworkCommsModel.h"
#include "../../model/data_models/WorkItems/HaltWorkItem/HaltWorkItem.h"
#include "../HeavySchedulingFunctions/HeavySchedulingFunctions.h"
#include "../../model/data_models/WorkItems/ProcessingItem/LowProcessingItem/LowProcessingItem.h"
#include "../../model/data_models/NetworkCommsModels/LowComplexityAllocation/LowComplexityAllocationComms.h"
#include "../../model/data_models/WorkItems/PruneItem/PruneItem.h"
#include "../../model/data_models/TimeSourceSingleton/TimeSourceSingleton.h"
#include "../LightSchedulingFunctions/LightSchedulingFunctions.h"
#include "../NetworkLinkDiscFunctions/NetworkDiscFunctions.h"
#include "../../Constants/RequestSizes.h"
#include "../../Constants/NETWORK_DISC_PARAMS.h"
#include "../../model/data_models/WorkItems/BandwidthUpdate/BandwidthUpdateItem.h"

namespace services {

	void state_update_call(std::shared_ptr<model::WorkItem> item, WorkQueueManager *queueManager, bool isLight) {
		std::shared_ptr<model::StateUpdate> stateUpdate = std::static_pointer_cast<model::StateUpdate>(item);

		std::unique_lock<std::mutex> off_lock(queueManager->offloaded_lock, std::defer_lock);
		off_lock.lock();

		auto &total_offload_list = queueManager->off_total;
		auto &total_off_high = queueManager->off_high;
		auto &total_off_low = queueManager->off_low;

		auto dnn_id = stateUpdate->getDnnId();
		auto dnn = total_offload_list[dnn_id];
		std::string allocated_device = dnn->getAllocatedHost();


		std::unique_lock<std::mutex> net_lock(queueManager->network_lock, std::defer_lock);
		net_lock.lock();

		auto device = queueManager->network->devices[allocated_device];
		auto &network_link = queueManager->network->network_link;

		services::state_update_function(total_offload_list, total_off_low, total_off_high, dnn_id, dnn, device,
										network_link, stateUpdate->isSuccess(), stateUpdate->getFinishTime());

		net_lock.unlock();
		off_lock.unlock();

		web::json::value log;

		if (dnn->getDnnType() == enums::dnn_type::low_comp) {
			log["dnn_details"] = std::static_pointer_cast<model::LowCompResult>(dnn)->convertToJson();
			queueManager->logManager->add_log(enums::LogTypeEnum::LOW_COMP_FINISH, log);
		} else {
			log["dnn_details"] = std::static_pointer_cast<model::HighCompResult>(dnn)->convertToJson();
			queueManager->logManager->add_log(stateUpdate->isSuccess() ? enums::LogTypeEnum::HIGH_COMP_FINISH
																	   : enums::LogTypeEnum::VIOLATED_DEADLINE, log);
		}


		queueManager->decrementThreadCounter();
	}

	/*Function receives alow complexity DNN allocation request*/
	void low_comp_allocation_call(std::shared_ptr<model::WorkItem> item, WorkQueueManager *queueManager, bool isLight) {
		auto proc_item = std::static_pointer_cast<model::LowProcessingItem>(item);
		auto [dnn_id, device] = proc_item->getDnnIdAndDevice();

		std::unique_lock<std::mutex> net_lock(queueManager->network_lock, std::defer_lock);
		net_lock.lock();
		std::map<std::string, std::shared_ptr<model::ComputationDevice>> &devices = queueManager->network->devices;

		net_lock.unlock();

		/* Fetching bytes and latency from the stored net parameters from iperf test */
		double bw_bytes = (queueManager->getAverageBitsPerSecond() / 8);
		double latency_bytes = queueManager->getJitter() / 8;
		bw_bytes += latency_bytes;

		if (isLight) {
			auto [result_state, allocated_time_window, resulting_task] = services::light_sched_low_comp_allocation_call(
					dnn_id, device, proc_item->getDeadline(), devices, devices[device], proc_item->isReallocation(),
					bw_bytes);

			if (result_state == ResultState::success || result_state == ResultState::post_preemp_succ) {
				auto bR = resulting_task;

				queueManager->off_low[bR->getDnnId()] = bR;
				queueManager->off_total[bR->getDnnId()] = bR;

				std::shared_ptr<model::LowComplexityAllocationComms> baseNetworkCommsModel = std::make_shared<model::LowComplexityAllocationComms>(
						enums::network_comms_types::low_complexity_allocation, bR->estimated_start_fin->start, bR,
						bR->getSrcHost());


				web::json::value log;
				log["dnn_details"] = bR->convertToJson();
				queueManager->logManager->add_log(
						proc_item->isReallocation() ? enums::LogTypeEnum::LOW_COMP_PREMPT_ALLOCATION_SUCCESS
													: enums::LogTypeEnum::LOW_COMP_ALLOCATION_SUCCESS, log);
				queueManager->networkQueueManager->addTask(baseNetworkCommsModel);
				net_lock.unlock();

				queueManager->decrementThreadCounter();
				return;
			} else if (result_state == ResultState::no_resources || result_state == ResultState::deadline_violation) {

				if (result_state == ResultState::no_resources) {
					std::shared_ptr<model::WorkItem> workItem = std::make_shared<model::HaltWorkItem>(
							enums::request_type::halt_req, device, allocated_time_window->start,
							allocated_time_window->stop);

					queueManager->add_task(workItem);
					auto newTask = std::make_shared<model::LowProcessingItem>(item->getHostList(),
																			  enums::request_type::low_complexity,
																			  proc_item->getDeadline(),
																			  proc_item->getDnnIdAndDevice());
					newTask->setReallocation(true);
					queueManager->add_task(std::static_pointer_cast<model::WorkItem>(newTask));

					web::json::value halt_log;
					halt_log["dnn_id"] = web::json::value::string(newTask->getDnnIdAndDevice().first);
					queueManager->logManager->add_log(enums::LogTypeEnum::HALT_REQUEST, halt_log);
				}

				web::json::value log;
				log["dnn_id"] = web::json::value::string(proc_item->getDnnIdAndDevice().first);
				log["current_time"] = web::json::value::number(
						model::TimeSourceSingleton::getTime().time_since_epoch().count() * 1000);
				log["source_device"] = web::json::value::string(device);
				log["deadline"] = web::json::value::number(proc_item->getDeadline().time_since_epoch().count() * 1000);
				log["network"] = queueManager->network->convertToJson();
				queueManager->logManager->add_log(
						(result_state == ResultState::deadline_violation) ? enums::LogTypeEnum::VIOLATED_DEADLINE
																		  : enums::LogTypeEnum::LOW_COMP_ALLOCATION_FAIL,
						log);
				queueManager->decrementThreadCounter();
				return;
			}
		} else {
			auto [low_comp_result, time_window] = services::low_comp_allocation_func(bw_bytes, queueManager->network,
																					 device, item);

			if (low_comp_result == nullptr) {
#if DEADLINE_PREMPT
				std::shared_ptr<model::WorkItem> workItem = std::make_shared<model::HaltWorkItem>(
						enums::request_type::halt_req, device, time_window->start, time_window->stop);

				queueManager->add_task(workItem);
				auto newTask = std::make_shared<model::LowProcessingItem>(item->getHostList(),
																		  enums::request_type::low_complexity,
																		  proc_item->getDeadline(),
																		  proc_item->getDnnIdAndDevice());
				newTask->setReallocation(true);
				queueManager->add_task(std::static_pointer_cast<model::WorkItem>(newTask));

				web::json::value halt_log;
				halt_log["dnn_id"] = web::json::value::string(newTask->getDnnIdAndDevice().first);
				queueManager->logManager->add_log(enums::LogTypeEnum::HALT_REQUEST, halt_log);
#endif
				web::json::value log;
				log["dnn_id"] = web::json::value::string(proc_item->getDnnIdAndDevice().first);
				log["current_time"] = web::json::value::number(
						model::TimeSourceSingleton::getTime().time_since_epoch().count() * 1000);
				log["source_device"] = web::json::value::string(device);
				log["deadline"] = web::json::value::number(proc_item->getDeadline().time_since_epoch().count() * 1000);
				log["network"] = queueManager->network->convertToJson();

				queueManager->logManager->add_log(enums::LogTypeEnum::LOW_COMP_ALLOCATION_FAIL, log);
				queueManager->decrementThreadCounter();

				return;
			} else {
				/* For each allocated low comp task we create a Result object
				 * the key is the dnn_id*/
				auto bR = low_comp_result;

				queueManager->off_low[bR->getDnnId()] = bR;
				queueManager->off_total[bR->getDnnId()] = bR;

				queueManager->network->devices[bR->getSrcHost()]->DNNS.push_back(bR);

				std::shared_ptr<model::LowComplexityAllocationComms> baseNetworkCommsModel = std::make_shared<model::LowComplexityAllocationComms>(
						enums::network_comms_types::low_complexity_allocation, bR->estimated_start_fin->start, bR,
						bR->getSrcHost());


				web::json::value log;
				log["dnn_details"] = bR->convertToJson();

				queueManager->logManager->add_log(proc_item->isReallocation()
												  ? enums::LogTypeEnum::LOW_COMP_PREMPT_ALLOCATION_SUCCESS
												  : enums::LogTypeEnum::LOW_COMP_ALLOCATION_SUCCESS, log);
				queueManager->networkQueueManager->addTask(baseNetworkCommsModel);

				queueManager->network->devices[bR->getAllocatedHost()]->resAvailRemoveAndSplit(bR->estimated_start_fin,
																							   bR->getCoreAllocation(), 0);


				net_lock.unlock();

			}
			queueManager->decrementThreadCounter();
		}
	}

	void
	high_comp_allocation_call(std::shared_ptr<model::WorkItem> item, WorkQueueManager *queueManager, bool isLight) {
		auto processingItem = std::static_pointer_cast<model::HighProcessingItem>(item);
		bool isReallocation = processingItem->isReallocation();

		std::string sourceHost = (*item->getHostList())[0];

		std::map<std::string, std::shared_ptr<model::HighCompResult>> baseResult;

		auto bytes_per_ms = queueManager->getBytesPerMillisecond();

		std::unique_lock<std::mutex> netlock(queueManager->network_lock, std::defer_lock);
		netlock.lock();
		std::unique_lock<std::mutex> offload_lock(queueManager->offloaded_lock, std::defer_lock);
		offload_lock.lock();

		std::map<std::string, std::shared_ptr<model::ComputationDevice>> copyDeviceWorkloadList;

		for (const auto &[key, value]: queueManager->network->devices) {
			auto temp = std::make_shared<model::ComputationDevice>(value->getCores(), value->getHostName());
			for (const auto &task: value->DNNS)
				temp->DNNS.push_back(task);
			copyDeviceWorkloadList[key] = temp;
		}

		/*Need to create a copy of the network list so that we can keep track of incomplete net allocations */
		std::vector<std::shared_ptr<model::NetCommunicationBase>> copyList;
		for (const auto &i: queueManager->network->network_link) {
			copyList.push_back(i);
		}

		std::vector<std::shared_ptr<model::HighCompResult>> resultItems;

		if (isLight) {
			std::vector<std::tuple<ResultState, std::string, std::shared_ptr<model::HighCompResult>>> tupleVectItems = services::light_sched_high_comp_allocation_call(
					isReallocation,
					processingItem->getDnnIds(),
					bytes_per_ms,
					sourceHost,
					processingItem->getDnnId(),
					queueManager->network->disc_network_link,
					processingItem->getDeadline(),
					queueManager->network->last_time_of_reasoning,
					queueManager->network->devices);

			for (const auto &[result_state, dnn_id_result, result_obj]: tupleVectItems) {
				if (result_state == ResultState::success || result_state == ResultState::post_preemp_succ) {
					resultItems.emplace_back(result_obj);
				}
			}


		} else {
			resultItems = services::high_comp_allocation_func(
					processingItem->isReallocation(), processingItem, queueManager->off_total, copyDeviceWorkloadList,
					copyList, bytes_per_ms, sourceHost);
		}
		for (const auto &task: resultItems) {
			std::shared_ptr<model::HighComplexityAllocationComms> high_comp_comms = std::make_shared<model::HighComplexityAllocationComms>(
					enums::network_comms_types::high_complexity_task_mapping, task->estimated_start_fin->start, task,
					task->getSrcHost());

			std::shared_ptr<model::BaseNetworkCommsModel> comm_task = std::static_pointer_cast<model::BaseNetworkCommsModel>(
					high_comp_comms);

			queueManager->networkQueueManager->addTask(comm_task);
		}
		for (const auto &task: resultItems) {
			if (task->getAllocatedHost() != task->getSrcHost()) {
				queueManager->network->addComm(task->task_allocation);

				if (isLight) {
					/* Marking bucket as occupied */
					auto comm = std::static_pointer_cast<model::Bucket>(task->task_allocation);

					comm->bucketContents.push_back(task->getDnnId());
				} else {
					auto comm = std::static_pointer_cast<model::LinkAct>(task->task_allocation);
					auto network = queueManager->network;
					auto start_time = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
							comm->timeWindow->start.time_since_epoch()).count());
					auto last_time_of_reasoning = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
							network->last_time_of_reasoning.time_since_epoch()).count());
					auto base_comm_size = static_cast<uint64_t>(TASK_FORWARD_SIZE / bytes_per_ms);
					uint64_t index = services::obtain_index(start_time, last_time_of_reasoning, base_comm_size,
															BASE_BUCKET_COUNT);

					for (uint64_t i = index; i < network->disc_network_link.size(); i++) {
						if (comm->timeWindow->start < network->disc_network_link[i]->timeWindow->stop &&
							comm->timeWindow->stop > network->disc_network_link[i]->timeWindow->start) {
							network->disc_network_link[i]->bucketContents.push_back(task->getDnnId());
						} else {
							break;
						}
					}
				}
			}

			queueManager->off_total[task->getDnnId()] = task;
			queueManager->off_high[task->getDnnId()] = task;

			auto device = queueManager->network->devices[task->getAllocatedHost()];
			device->DNNS.push_back(task);

			web::json::value log;
			log["dnn"] = web::json::value(task->convertToJson());
			queueManager->logManager->add_log((isReallocation) ? enums::LogTypeEnum::HIGH_COMP_REALLOCATION_SUCCESS
															   : enums::LogTypeEnum::HIGH_COMP_ALLOCATION_SUCCESS, log);


			device->resAvailRemoveAndSplit(task->estimated_start_fin, task->getCoreAllocation(), 0);

		}


		for (const auto &dnn_id: processingItem->getDnnIds()) {
			bool allocated = false;
			for (const auto &dnn_task: resultItems) {
				if (dnn_id == dnn_task->getDnnId()) {
					allocated = true;
					break;
				}
			}

			if (!allocated) {
				web::json::value log;
				log["dnn_id"] = web::json::value::string(dnn_id);
				queueManager->logManager->add_log((isReallocation) ? enums::LogTypeEnum::HIGH_COMP_REALLOCATION_FAIL
																   : enums::LogTypeEnum::HIGH_COMP_ALLOCATION_FAIL,
												  log);
			}
		}

		netlock.unlock();
		offload_lock.unlock();
		queueManager->decrementThreadCounter();
	}

	void halt_call(std::shared_ptr<model::WorkItem> workItem, WorkQueueManager *queueManager, bool isLight) {
		auto haltWorkItem = std::static_pointer_cast<model::HaltWorkItem>(workItem);
		std::vector<std::string> hostList;
		for (auto [host_id, host]: queueManager->network->getDevices()) {
			hostList.push_back(host_id);
		}

		auto sourceDeviceId = haltWorkItem->getHostToExamine();
		auto startTime = haltWorkItem->getStartTime();
		auto finTime = haltWorkItem->getFinTime();

		std::unique_lock<std::mutex> offload_lock(queueManager->offloaded_lock, std::defer_lock);
		std::unique_lock<std::mutex> network_lock(queueManager->network_lock, std::defer_lock);
		offload_lock.lock();
		network_lock.lock();

		auto [dnnToPrune, deviceTaskList, networkLinkCopy, totalOffloadCopy, totalHighCopy, versionToPrune] = services::halt_call_func(
				haltWorkItem, queueManager->network->devices[sourceDeviceId]->DNNS, queueManager->network->network_link,
				queueManager->off_total, queueManager->off_high);

		queueManager->off_total = totalOffloadCopy;
		queueManager->off_high = totalHighCopy;
		queueManager->network->network_link = networkLinkCopy;
		queueManager->network->devices[sourceDeviceId]->DNNS = deviceTaskList;

		std::shared_ptr<model::HaltNetworkCommsModel> baseNetworkCommsModel = std::make_shared<model::HaltNetworkCommsModel>(
				enums::network_comms_types::halt_req, model::TimeSourceSingleton::getTime(), sourceDeviceId,
				dnnToPrune->getDnnId(), versionToPrune);

		queueManager->networkQueueManager->addTask(baseNetworkCommsModel);

		network_lock.unlock();
		offload_lock.unlock();

		/* We attempt to reallocate the DNN if possible */
		auto host_list = std::make_shared<std::vector<std::string>>(
				std::initializer_list<std::string>{dnnToPrune->getSrcHost()});

		std::shared_ptr<model::HighProcessingItem> highProcessingItem = std::make_shared<model::HighProcessingItem>(
				host_list, enums::request_type::high_complexity, dnnToPrune->getDeadline(),
				std::string(dnnToPrune->getDnnId()), std::initializer_list<std::string>{dnnToPrune->getDnnId()});

		highProcessingItem->setReallocation(true);
		queueManager->add_task(highProcessingItem);

		if (isLight) {
			auto regenDataStr = std::make_shared<model::WorkItem>(
					std::make_shared<std::vector<std::string>>(std::vector<std::string>{sourceDeviceId}),
					enums::request_type::regenerate_structure);

			queueManager->add_task(regenDataStr);
		}
		queueManager->decrementThreadCounter();
	}

	void regenerate_res_data_structure(std::shared_ptr<model::WorkItem> workItem, WorkQueueManager *queueManager) {
		auto host = (*workItem->getHostList())[0];
		auto device = queueManager->network->devices[host];

		services::light_sched_regenerate_res_data_structure(host, device);

		queueManager->decrementThreadCounter();
	}

	void
	update_network_disc(std::shared_ptr<model::WorkItem> workItem, WorkQueueManager *queueManager) {
		auto bps = queueManager->getBytesPerMillisecond();
		auto disc_link = queueManager->network->disc_network_link;

		auto [result_link, last_time_of_reasoning] = services::light_sched_update_network_disc(bps, disc_link);

		queueManager->network->disc_network_link = result_link;
		queueManager->network->last_time_of_reasoning = last_time_of_reasoning;

		queueManager->decrementThreadCounter();
	}

	void
	update_bw_vals(std::shared_ptr<model::WorkItem> workItem, WorkQueueManager *queueManager) {
		auto bandwidth_update = std::static_pointer_cast<model::BandwidthUpdateItem>(workItem);
		auto bits_per_second_vect = bandwidth_update->bits_per_second_vect;

		auto bps = queueManager->getAverageBitsPerSecond();
		auto jit = queueManager->getJitter();

		auto [new_bps, new_jitter] = services::light_sched_update_bw_vals(bits_per_second_vect, bps, jit);
		queueManager->setAverageBitsPerSecond(new_bps);
		queueManager->setJitter(new_jitter);

		queueManager->decrementThreadCounter();
	}

	bool calculate_load_switch(std::chrono::time_point<std::chrono::system_clock> current_time, std::chrono::time_point<std::chrono::system_clock> deadline){
		return false;
	}

} // services