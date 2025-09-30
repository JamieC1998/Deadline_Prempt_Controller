//
// Created by jamiec on 9/23/22.
//

#include <thread>
#include <utility>
#include "WorkQueueManager.h"
#include "../../model/data_models/WorkItems/ProcessingItem/HighProcessingItem/HighProcessingItem.h"
#include "../../model/data_models/WorkItems/ProcessingItem/LowProcessingItem/LowProcessingItem.h"
#include "../../model/data_models/NetworkCommsModels/LowComplexityAllocation/LowComplexityAllocationComms.h"
#include "../../model/data_models/WorkItems/PruneItem/PruneItem.h"
#include "../WorkQueueFunctions/WorkQueueFunctions.h"
#include "../../model/data_models/TimeSourceSingleton/TimeSourceSingleton.h"
#include "../LightSchedulingFunctions/LightSchedulingFunctions.h"
#include "../../Constants/NETWORK_DISC_PARAMS.h"
#include "../../model/data_models/NetworkCommsModels/BandwidthTestCommsModel/BandwidthTestCommsModel.h"
#include "../../Constants/CLIENT_DETAILS.h"
#include "../ProfilerMain/ProfilerMain.h"

namespace services {
	std::atomic<int> WorkQueueManager::thread_counter = 0;

	void WorkQueueManager::add_task(std::shared_ptr<model::WorkItem> item) {
		web::json::value log;
		std::unique_lock<std::mutex> lk(WorkQueueManager::work_queue_lock, std::defer_lock);
		lk.lock();
		WorkQueueManager::logManager->add_log(enums::LogTypeEnum::ADD_WORK_TASK, log);
		WorkQueueManager::work_queue.push_back(item);

		std::sort(WorkQueueManager::work_queue.begin(), WorkQueueManager::work_queue.end(),
				  [](std::shared_ptr<model::WorkItem> a, std::shared_ptr<model::WorkItem> b) {

					  if (a->getRequestType() == b->getRequestType()) {
						  if (a->getRequestType() == enums::request_type::low_complexity) {
							  auto a_cast = std::static_pointer_cast<model::LowProcessingItem>(a);
							  auto b_cast = std::static_pointer_cast<model::LowProcessingItem>(b);

							  return a_cast->getDeadline() < b_cast->getDeadline();
						  } else if (a->getRequestType() == enums::request_type::high_complexity) {
							  auto a_cast = std::static_pointer_cast<model::HighProcessingItem>(a);
							  auto b_cast = std::static_pointer_cast<model::HighProcessingItem>(b);

							  return a_cast->getDeadline() < b_cast->getDeadline();
						  }
						  return true;
					  } else
						  return a->getRequestType() < b->getRequestType();
				  });
		lk.unlock();
	}

	[[noreturn]] void WorkQueueManager::main_loop(WorkQueueManager *queueManager) {
		std::chrono::time_point<std::chrono::system_clock> next_discretisation_task;
		std::random_device rd;
		std::mt19937 gen(rd());
		try {
			volatile bool wait = false;
			while (queueManager->networkQueueManager->getHosts().size() != CLIENT_COUNT) {
				wait;
				continue;
			}

			std::cout << "Beginning Experiment" << std::endl;
			update_network_disc(nullptr, queueManager);

			next_discretisation_task =
					std::chrono::system_clock::now() + std::chrono::milliseconds{NETWORK_PING_FREQ_MS};

			std::unique_lock<std::mutex> lk(queueManager->work_queue_lock, std::defer_lock);

			bool isLight = false;

			while (true) {
				queueManager->current_task.clear();
				queueManager->thread_counter = 0;

				if (!queueManager->work_queue.empty()) {
					lk.lock();

					queueManager->thread_counter++;
					queueManager->current_task.push_back(queueManager->work_queue.front());
					queueManager->work_queue.erase(queueManager->work_queue.begin());

					lk.unlock();


					std::vector<std::thread> thread_pool;

					if (queueManager->current_task.front()->getRequestType() == enums::request_type::low_complexity || queueManager->current_task.front()->getRequestType() == enums::request_type::high_complexity) {
						std::chrono::
						time_point<std::chrono::system_clock> deadline;
						auto load = services::numberOfTasksInNetwork(queueManager->network);

						auto time_to_uint64_t = static_cast<uint64_t>(std::llround(
								queueManager->load_latency_map[load].first));

						auto estimated_scheduling_time = std::chrono::milliseconds{time_to_uint64_t} + std::chrono::system_clock::now();

						if (queueManager->current_task.front()->getRequestType() == enums::request_type::low_complexity) {
							auto work_item = std::static_pointer_cast<model::HighProcessingItem>(
									queueManager->current_task.front());

							deadline = work_item->getDeadline();
						} else {
							auto work_item = std::static_pointer_cast<model::LowProcessingItem>(
									queueManager->current_task.front());

							deadline = work_item->getDeadline();
						}

						isLight = services::calculate_load_switch(estimated_scheduling_time, deadline);
					}

					switch (queueManager->current_task.front()->getRequestType()) {
						case enums::request_type::low_complexity: {


							thread_pool.emplace_back(services::low_comp_allocation_call,
													 queueManager->current_task.front(),
													 queueManager, isLight);
						}
							break;
						case enums::request_type::high_complexity: {
							thread_pool.emplace_back(services::high_comp_allocation_call,
													 queueManager->current_task.front(),
													 queueManager, isLight);
						}
							break;
						case enums::request_type::halt_req:
							thread_pool.emplace_back(services::halt_call, queueManager->current_task.front(),
													 queueManager, false);
							break;
						case enums::request_type::state_update:
							thread_pool.emplace_back(state_update_call, queueManager->current_task.front(),
													 queueManager, false);
							break;
						case enums::request_type::bandwidth_update:
							thread_pool.emplace_back(services::update_bw_vals, queueManager->current_task.front(),
													 queueManager);
							break;
						case enums::request_type::network_disc:
							thread_pool.emplace_back(services::update_network_disc, queueManager->current_task.front(),
													 queueManager);
							break;
						case enums::request_type::regenerate_structure:
							thread_pool.emplace_back(services::regenerate_res_data_structure, queueManager->current_task.front(),
													 queueManager);
							break;
					}
					thread_pool.back().detach();

					/* We wait for the current work items to terminate */
					while (queueManager->thread_counter > 0) {
						lk.lock();
						if (!queueManager->work_queue.empty() && queueManager->current_task.front()->getRequestType() ==
																 enums::request_type::low_complexity &&
							queueManager->work_queue.front()->getRequestType() ==
							enums::request_type::low_complexity) {
							/* If more low complexity tasks are added to the queue then we append them to the current
							 * work list */
							while (!queueManager->work_queue.empty() &&
								   queueManager->work_queue.front()->getRequestType() ==
								   enums::request_type::low_complexity) {
								queueManager->thread_counter++;
								thread_pool.emplace_back(low_comp_allocation_call, queueManager->work_queue.front(),
														 queueManager, false);
								thread_pool.back().detach();
								queueManager->current_task.push_back(queueManager->work_queue.front());
								queueManager->work_queue.erase(queueManager->work_queue.begin());
							}
						}
						lk.unlock();
					}
					thread_pool.clear();
				}

				if (std::chrono::system_clock::now() > next_discretisation_task) {
					lk.lock();

					auto network_device_list = queueManager->networkQueueManager->hosts;

					std::uniform_int_distribution<> dis(0, static_cast<int>(network_device_list.size()) -
														   1); // Range from 0 to vec.size()-1

					auto chosen_host = network_device_list[dis(gen)];

					std::shared_ptr<model::BandwidthTestCommsModel> bandwidth_test = std::make_shared<model::BandwidthTestCommsModel>(
							enums::network_comms_types::bandwidth_update, std::chrono::system_clock::now(),
							chosen_host);

					queueManager->networkQueueManager->addTask(
							std::static_pointer_cast<model::BaseNetworkCommsModel>(bandwidth_test));
					next_discretisation_task =
							std::chrono::system_clock::now() + std::chrono::milliseconds{NETWORK_PING_FREQ_MS};
					lk.unlock();
				}
			}
		}
		catch (std::exception &e) {
			std::cerr << "WORK_QUEUE_MANAGER: something wrong has happened! ;)" << '\n';
			std::cerr << e.what() << "\n";
		}
	}

	void WorkQueueManager::decrementThreadCounter() {
		WorkQueueManager::thread_counter--;
	}

	double WorkQueueManager::getAverageBitsPerSecond() const {
		return average_bits_per_second;
	}

	void WorkQueueManager::setAverageBitsPerSecond(double averageBitsPerSecond) {
		average_bits_per_second = averageBitsPerSecond;
	}

	double WorkQueueManager::getJitter() const {
		return jitter;
	}

	void WorkQueueManager::setJitter(double jitter) {
		WorkQueueManager::jitter = jitter;
	}

	WorkQueueManager::WorkQueueManager(std::shared_ptr<LogManager>
									   ptr, std::shared_ptr<NetworkQueueManager>
									   sharedPtr, std::map<int, std::pair<float, std::vector<uint64_t>>> load_latency_map)
			: logManager(std::move(ptr)) {
		WorkQueueManager::network = std::make_shared<model::Network>();
		WorkQueueManager::networkQueueManager = sharedPtr;
		WorkQueueManager::load_latency_map = std::move(load_latency_map);

	}

	double WorkQueueManager::getBytesPerMillisecond() const {
		/* Fetching the bandwidth and latency */
		double bw_bytes_per_second = (WorkQueueManager::getAverageBitsPerSecond() / 8);
		double latency_bytes = WorkQueueManager::getJitter() / 8;
		bw_bytes_per_second -= latency_bytes;
		double bw_bytes_per_ms = bw_bytes_per_second / 1000;
		return bw_bytes_per_ms;
	}

} // services