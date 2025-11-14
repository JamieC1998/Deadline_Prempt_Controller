//
// Created by Jamie Cotter on 28/07/2025.
//

#include "LightSchedulingFunctions.h"
#include <thread>
#include "../../Constants/EMA.h"
#include "../../Constants/RequestSizes.h"
#include "../../Constants/NETWORK_DISC_PARAMS.h"
#include "../../Constants/CLIENT_DETAILS.h"
#include "../NetworkLinkDiscFunctions/NetworkDiscFunctions.h"
#include "../../model/data_models/TimeSourceSingleton/TimeSourceSingleton.h"
#include "../../Constants/AllocationMacros.h"
#include "../../Constants/FTP_CONFIG.h"

namespace services {

	std::pair<uint64_t, uint64_t>
	light_sched_update_bw_vals(const std::vector<double> &bits_per_second_vect, double averageBPS, double jitter) {
		double new_bps = averageBPS;
		double new_jitter = jitter;

		for (double i: bits_per_second_vect) {
			auto old_bps = new_bps;
			auto old_jitter = new_jitter;

			auto diff = i - old_bps;
			auto increment = EMA_ALPHA * diff;

			auto mean = old_bps + increment;
			auto variance = (1 - EMA_ALPHA) * (old_jitter + diff * increment);

			new_bps = mean;
			new_jitter = variance;
		}

		return std::make_pair(new_bps, new_jitter);
	}

	std::pair<std::vector<std::shared_ptr<model::Bucket>>, std::chrono::time_point<std::chrono::system_clock>>
	light_sched_update_network_disc(double bytes_per_millisecond, std::vector<std::shared_ptr<model::Bucket>> disc_link) {
		auto base_comm_size = static_cast<uint64_t>(TASK_FORWARD_SIZE / bytes_per_millisecond);

		std::chrono::time_point<std::chrono::system_clock> current_time_of_reasoning = model::TimeSourceSingleton::getTime();

		std::vector<std::shared_ptr<model::Bucket>> new_network_list = services::cascade_function(
				current_time_of_reasoning, BASE_BUCKET_COUNT, disc_link, base_comm_size,
				EXP_BUCKET_COUNT);

		return std::make_pair(new_network_list, current_time_of_reasoning);
	}

	/*Function receives a low complexity DNN allocation request*/
	std::tuple<ResultState, std::shared_ptr<model::TimeWindow>, std::shared_ptr<model::LowCompResult>>
	light_sched_low_comp_allocation_call(std::string dnn_id, std::string host,
										 std::chrono::time_point<std::chrono::system_clock> deadline,
										 std::map<std::string, std::shared_ptr<model::ComputationDevice>> devices,
										 std::shared_ptr<model::ComputationDevice> device,
										 bool isReallocation,
										 double bw_bytes) {

		if (deadline < model::TimeSourceSingleton::getTime()) {
			return std::make_tuple(ResultState::deadline_violation, nullptr, nullptr);
		}

		uint64_t data_size_bytes = LOW_TASK_SIZE;
		/* -------------------------------------------------------------------------- */
		std::chrono::time_point<std::chrono::system_clock> currentTime = model::TimeSourceSingleton::getTime() +
																		 std::chrono::milliseconds{
																				 static_cast<uint64_t>(data_size_bytes /
																									   bw_bytes)} +
																		 std::chrono::milliseconds{
																				 LOW_COMP_START_OFFSET_MS};

		auto estimated_fin = currentTime + std::chrono::milliseconds{LOW_COMPLEXITY_PROCESSING_TIME};

		auto res = device->resource_avail_windows[1]->containmentQuery(
				currentTime, estimated_fin);

		auto index = res.first;
		auto window = res.second;

		/* If we cannot allocate a device for even one task we instead create a new allocation request and a halt request */
		if (index == TASK_NOT_FOUND)
			return std::make_tuple(ResultState::no_resources, std::make_shared<model::TimeWindow>(currentTime, estimated_fin), nullptr);

		/* For each allocated low comp task we create a Result object
		 * the key is the dnn_id*/

		auto bR = std::make_shared<model::LowCompResult>(dnn_id, host, 1, deadline, currentTime,
														 estimated_fin,
														 enums::dnn_type::low_comp
		);

		/* Add communication times to the network link */
		bR->setAllocatedHost(host);

		device->DNNS.push_back(bR);

		device->resAvailRemoveAndSplit(bR->estimated_start_fin, LOW_COMPLEXITY_CORE_COUNT, 0);

		return std::make_tuple((isReallocation) ? ResultState::post_preemp_succ : ResultState::success,
							   window->timeWindow, bR);
	}

	int selectResourceConfig(std::chrono::time_point<std::chrono::system_clock> start,
							 std::chrono::time_point<std::chrono::system_clock> deadline) {
		int resourceConfig = 2;
		if ((deadline - start) < std::chrono::milliseconds{FTP_LOW_TIME}) {
			resourceConfig = 4;
			if ((deadline - start) < std::chrono::milliseconds{FTP_HIGH_TIME}) {
				resourceConfig = TASK_NOT_FOUND;
			}
		}
		return resourceConfig;
	}

	std::vector<int> gatherCommSlots(int task_count, std::vector<std::shared_ptr<model::Bucket>> netlink,
									 std::chrono::time_point<std::chrono::system_clock> currentTime,
									 std::chrono::time_point<std::chrono::system_clock> last_time_of_reasoning,
									 double bytes_per_millisecond
	) {
		/* Gather Comm Windows */
		std::vector<int> task_comm_windows = {};

		auto ct_uint = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
				currentTime.time_since_epoch()).count());
		auto last_reason_uint = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
				last_time_of_reasoning.time_since_epoch()).count());
		auto base_comm_size = static_cast<uint64_t>(TASK_FORWARD_SIZE / bytes_per_millisecond);

		for (int i = 0; i < task_count; i++) {
			uint64_t index = services::obtain_index(ct_uint, last_reason_uint, base_comm_size, BASE_BUCKET_COUNT);
			while (netlink[index]->bucketContents.size() == netlink[index]->capacity)
				index += 1;

			currentTime = netlink[index]->timeWindow->stop;
			ct_uint = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
					currentTime.time_since_epoch()).count());
			task_comm_windows.push_back(static_cast<int>(index));
		}
		return task_comm_windows;
	}

	void findResourceWindow(std::chrono::time_point<std::chrono::system_clock> start_time,
							std::chrono::time_point<std::chrono::system_clock> deadline,
							std::shared_ptr<model::ComputationDevice> device, int resourceConfig, int task_count,
							std::string host,
							std::map<std::string, std::vector<std::pair<int, std::shared_ptr<model::ResourceWindow>>>> &results,
							std::mutex &mtx) {
		auto res = device->resource_avail_windows[resourceConfig]->containmentQueryMulti(start_time, deadline,
																						 task_count);

		std::lock_guard<std::mutex> guard(mtx);
		results[host] = res;
	}

	std::pair<std::vector<std::pair<int, std::shared_ptr<model::ResourceWindow>>>, std::vector<std::pair<int, std::shared_ptr<model::ResourceWindow>>>>
	selectResults(
			std::map<std::string, std::vector<std::pair<int, std::shared_ptr<model::ResourceWindow>>>> placement_results,
			std::string sourceHost, int task_count) {
		std::vector<std::pair<int, std::shared_ptr<model::ResourceWindow>>> source_selected_results;
		std::vector<std::pair<int, std::shared_ptr<model::ResourceWindow>>> offloaded_selected_results;

		for (auto result: placement_results[sourceHost]) {
			if (source_selected_results.size() != task_count)
				source_selected_results.push_back(result);
			else
				break;
		}

		placement_results.erase(sourceHost);
		std::vector<std::vector<std::pair<int, std::shared_ptr<model::ResourceWindow>>>> placement_matrix;

		placement_matrix.reserve(placement_results.size());

		for (const auto &[host, placement_result]: placement_results)
			placement_matrix.push_back(placement_result);

		std::random_device rd;  // Obtain a random number from hardware
		std::mt19937 g(rd());   // Seed the generator

		std::shuffle(placement_matrix.begin(), placement_matrix.end(), g);  // Shuffle the array

		bool task_count_met = false;

		while (true) {  // Keep looping until we explicitly break
			bool all_empty = true;  // Track if all vectors are empty

			for (auto &result_vect: placement_matrix) {
				if (result_vect.empty())
					continue;  // Skip empty vectors, but don't break yet

				all_empty = false;  // At least one vector is non-empty

				const auto &result = result_vect.back();
				result_vect.pop_back();

				if (source_selected_results.size() + offloaded_selected_results.size() != task_count)
					offloaded_selected_results.push_back(result);

				if (source_selected_results.size() + offloaded_selected_results.size() == task_count) {
					task_count_met = true;
					break;
				}
			}

			if (all_empty || task_count_met)  // If all vectors were empty, exit the loop
				break;
		}

		return std::make_pair(source_selected_results, offloaded_selected_results);
	}

	std::tuple<std::vector<int>, std::vector<std::shared_ptr<model::HighCompResult>>, std::vector<std::shared_ptr<model::HighComplexityAllocationComms>>>
	generateHighCompResults(
			std::vector<std::string> dnnIds,
			std::vector<std::pair<int, std::shared_ptr<model::ResourceWindow>>> source_selected_results,
			std::vector<std::pair<int, std::shared_ptr<model::ResourceWindow>>> offloaded_selected_results,
			std::vector<std::shared_ptr<model::Bucket>> network_link,
			std::vector<int> task_comm_windows,
			std::string sourceHost,
			int resourceConfig,
			std::chrono::time_point<std::chrono::system_clock> currentTime,
			uint64_t processing_time,
			int n,
			int m,
			std::chrono::time_point<std::chrono::system_clock> deadline,
			bool isReallocation
	) {
		int source_select_counter = 0;
		int offloaded_select_counter = 0;

		std::vector<int> selected_link_results;
		std::vector<std::shared_ptr<model::HighCompResult>> high_comp_results;
		std::vector<std::shared_ptr<model::HighComplexityAllocationComms>> high_complexity_comms;
		std::chrono::time_point<std::chrono::system_clock> ct = currentTime;

		for (const auto &dnn_id: dnnIds) {
			std::shared_ptr<model::Bucket> selectedBucket = nullptr;
			std::shared_ptr<model::ResourceWindow> resource_window;
			if (source_selected_results.size() == source_select_counter &&
				offloaded_selected_results.size() == offloaded_select_counter)
				break;
			if (source_selected_results.size() != source_select_counter) {
				auto [res_index, res_window] = source_selected_results[source_select_counter];
				resource_window = res_window;
				source_select_counter++;
			} else {
				auto [res_index, res_window] = offloaded_selected_results[offloaded_select_counter];
				resource_window = res_window;
				const auto &bucket = network_link[task_comm_windows[offloaded_select_counter]];
				selectedBucket = bucket;
				ct = bucket->timeWindow->stop;
				selected_link_results.push_back(task_comm_windows[offloaded_select_counter]);
				offloaded_select_counter++;
			}

			auto high_comp_res = std::make_shared<model::HighCompResult>(dnn_id, resource_window->deviceId, sourceHost,
																		 resourceConfig, deadline,
																		 ct,
																		 ct +
																		 std::chrono::milliseconds{processing_time},
																		 enums::dnn_type::high_comp, n, m,
																		 selectedBucket);

			if (isReallocation)
				high_comp_res->setVersion(std::chrono::system_clock::now().time_since_epoch().count() * 1000);

			high_comp_results.push_back(high_comp_res);

			high_complexity_comms.push_back(std::make_shared<model::HighComplexityAllocationComms>(
					enums::network_comms_types::high_complexity_task_mapping,
					std::chrono::system_clock::now(),
					high_comp_res,
					sourceHost));
		}
		return std::make_tuple(selected_link_results, high_comp_results, high_complexity_comms);
	}

	std::tuple<int, int, uint64_t> selectResourceUsage(int resourceConfig) {
		int n = FTP_LOW_N;
		int m = FTP_LOW_M;
		uint64_t processing_time = FTP_LOW_TIME;
		if (resourceConfig == FTP_HIGH_CORE) {
			n = FTP_HIGH_N;
			m = FTP_HIGH_M;
			processing_time = FTP_HIGH_TIME;
		}

		return std::make_tuple(n, m, processing_time);
	}

	std::vector<std::tuple<ResultState, std::string, std::shared_ptr<model::HighCompResult>>>
	light_sched_high_comp_allocation_call(bool isReallocation, std::vector<std::string> dnn_ids,
										  double bw_bytes, std::string sourceHost, std::string dnn_id,
										  std::vector<std::shared_ptr<model::Bucket>> disc_link,
										  std::chrono::time_point<std::chrono::system_clock> deadline,
										  std::chrono::time_point<std::chrono::system_clock> last_time_of_reasoning,
										  std::map<std::string, std::shared_ptr<model::ComputationDevice>> devices) {

		int task_count = static_cast<int>(dnn_ids.size());
		std::map<std::string, std::shared_ptr<model::HighCompResult>> baseResult;

		std::chrono::time_point<std::chrono::system_clock> currentTime =
				model::TimeSourceSingleton::getTime() + std::chrono::milliseconds{HIGH_COMP_START_TIME_OFFSET_MS};

		/* Selecting which config to use based on processing viability */
		int resourceConfig = selectResourceConfig(currentTime, deadline);
		std::vector<std::tuple<ResultState, std::string, std::shared_ptr<model::HighCompResult>>> resultVector;
		if (resourceConfig == TASK_NOT_FOUND) {

			for (std::string id: dnn_ids)
				resultVector.emplace_back(ResultState::deadline_violation, id, nullptr);
			return resultVector;
		}

		/* Gathering the FTP Config based on core usage */
		auto [n, m, processing_time] = selectResourceUsage(resourceConfig);

		/* Gather Comm Windows */
		std::vector<int> task_comm_windows = gatherCommSlots(task_count, disc_link,
															 currentTime, last_time_of_reasoning,
															 bw_bytes);

		/* Find resource windows that overlap with our desired window */
		std::map<std::string, std::vector<std::pair<int, std::shared_ptr<model::ResourceWindow>>>> placement_results;

		std::mutex mtx;
		std::vector<std::thread> thread_pool;

		for (const auto &[hostname, device]: devices) {

			thread_pool.emplace_back(findResourceWindow, currentTime, deadline, device, resourceConfig, task_count,
									 hostname,
									 std::ref(placement_results), std::ref(mtx));
		}

		bool all_joined = false;
		while (!all_joined) {
			all_joined = true; // Assume all threads are joined initially.
			for (auto &thread_item: thread_pool) {
				if (thread_item.joinable()) {
					thread_item.join();
					all_joined = false; // A joinable thread was found and joined, so we keep looping.
				}
			}
		}

		/* Filtering the resource windows, prioritising local device */
		auto [source_selected_results, offloaded_selected_results] = selectResults(placement_results, sourceHost,
																				   task_count);


		/* If there are no valid resource windows, exit */
		if ((source_selected_results.empty() && offloaded_selected_results.empty()) ||
			(source_selected_results.size() + offloaded_selected_results.size() < task_count)) {
			auto state = (isReallocation) ? ResultState::no_resources_reallocation
										  : ResultState::no_resources;
			for (const auto &id: dnn_ids)
				resultVector.emplace_back(state, id, nullptr);
			return resultVector;
		}

		/* Generating state data structures */
		auto [selected_link_results, hcr, high_comp_comms] = generateHighCompResults(
				dnn_ids,
				source_selected_results,
				offloaded_selected_results,
				disc_link,
				task_comm_windows, sourceHost,
				resourceConfig, currentTime,
				processing_time, n, m, deadline, isReallocation);


		auto high_comp_results = hcr;

		/* LOGGING RESULTS and Adding RESULTS to network state */
		for (int i = 0; i < dnn_ids.size(); i++) {
			auto success = (isReallocation) ? ResultState::post_preemp_succ : ResultState::success;
			auto fail = (isReallocation) ? ResultState::no_resources_reallocation : ResultState::no_resources;
			if (i < high_comp_results.size())
				resultVector.emplace_back(success, dnn_ids[i], high_comp_results[i]);
			else
				resultVector.emplace_back(fail, dnn_ids[i], nullptr);

		}

		return resultVector;
	}

	std::shared_ptr<model::ComputationDevice> light_sched_regenerate_res_data_structure(std::string host, std::shared_ptr<model::ComputationDevice> device) {

		std::chrono::time_point<std::chrono::system_clock> ct = model::TimeSourceSingleton::getTime();

		device->generateDefaultResourceConfig(device->getCores(), device->getHostName(), ct);

		if (!device->DNNS.empty()) {

			for (const auto &dnn: device->DNNS) {
				device->resAvailRemoveAndSplit(dnn->estimated_start_fin, dnn->getCoreAllocation(), 0);
			}
		}

		return device;
	}
} // services