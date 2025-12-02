

#include <iostream>
#include "thread"
#include <string>
#include <cpprest/http_listener.h>
#include <cpprest/http_client.h>
#include "controller/MasterController.h"
#include "Constants/CLIENT_DETAILS.h"
#include "Services/ProfileFileReader/ProfileFileReader.h"
#include "Constants/LOG_CONSTANT.h"
#include "Services/ProfilerMain/ProfilerMain.h"
#include "model/data_models/IsProfilingSingleton/IsProfilingSingleton.h"
#include "utils/UtilFunctions/UtilFunctions.h"

int main(int argc, char *argv[]) {
	int device_count = 100;
	bool profile_light = true;
	std::string profiler_light_or_heavy = "false";

	if (argc > 6) {
		device_count = std::stoi(argv[1]);
		std::string controller_hostname = argv[2];
		std::string simulator_input_path = argv[3];
		std::string simulator_output_path = argv[4];
		std::string system_output_path = argv[5];
		profiler_light_or_heavy = argv[6];

		PROFILE_RESULT_FILE = simulator_output_path;
		PROFILE_INPUT_FILE = simulator_input_path;
		RESULTS_FILE = system_output_path;
		CONTROLLER_HOSTNAME = controller_hostname;
		profile_light = (profiler_light_or_heavy == "true");
	} else {
		std::cout <<
				"Run the program using the following config: ./controller int: simulated_device_count string: controller_hostname string: simulator_input_path string: simulation_output_path string: system_output_path bool {true, false}: profiler_heavy_or_light"
				<< std::endl;
		device_count = 100;
		PROFILE_INPUT_FILE = "/Users/jamiecotter/Documents/Work/PhD/Deadline_Prempt_Controller/weighted_4_slice_scheduler_preempt_old_bw.json";
		RESULTS_FILE = "/Users/jamiecotter/Documents/Work/PhD/Deadline_Prempt_Controller/result_log.json";
		PROFILE_RESULT_FILE = "/Users/jamiecotter/Documents/Work/PhD/Deadline_Prempt_Controller/heavy_profile_result.json";
		profile_light = (profiler_light_or_heavy == "true");
	}

	std::cout << utils::debugTimePointToString(std::chrono::system_clock::now()) << " - Running with the following config - ./controller " << std::to_string(device_count) << " " << CONTROLLER_HOSTNAME << " " <<
				PROFILE_INPUT_FILE << " " << PROFILE_RESULT_FILE << " " << RESULTS_FILE << " " << profiler_light_or_heavy << std::endl;

	try {
		auto jsonObj = services::read_experiment_log(PROFILE_INPUT_FILE);
		auto [state_u_list, simEventList] = services::parse_experiment_log(jsonObj, device_count);

		model::IsProfilingSingleton::setIsProfiling(true);
		auto profile_map = services::profiler_event_loop(state_u_list, simEventList, profile_light);
		auto load_transformation = services::profile_data_transform(profile_map);

		services::writeTransformedLoadResults(load_transformation, PROFILE_RESULT_FILE);

		std::shared_ptr<services::LogManager> logManager = std::make_shared<services::LogManager>();
		//		std::shared_ptr<services::NetworkQueueManager> networkQueueManager = std::make_shared<services::NetworkQueueManager>(
		//				logManager);
		//		auto *workQueueManager = new services::WorkQueueManager(
		//				logManager, networkQueueManager, load_transformation);
		//
		//		MasterController controller = MasterController(logManager, workQueueManager);
		//
		//		web::http::client::http_client_config cfg;
		//
		//		cfg.guarantee_order();
		//		web::http::experimental::listener::http_listener listener(
		//				"http://" + std::string(CONTROLLER_HOSTNAME) + ":" + std::to_string(CONTROLLER_REST_PORT) + "/controller");
		//		listener.support(web::http::methods::GET,
		//						 std::bind(&MasterController::handle_get, &controller, std::placeholders::_1));
		//		listener.support(web::http::methods::POST,
		//						 std::bind(&MasterController::handle_post, &controller, std::placeholders::_1));

		try {
			//			volatile int a = 0;
			//			listener.open().wait();
			//			std::cout << "Listening for requests at: " << listener.uri().to_string() << std::endl;
			//
			//			auto work_thread = std::thread(services::WorkQueueManager::main_loop, workQueueManager);
			//			work_thread.detach();
			//			auto network_thread = std::thread(services::NetworkQueueManager::initNetworkCommLoop, networkQueueManager);
			//			network_thread.detach();
			//
			//			while (true) a;
		} catch (std::exception &e) {
			std::cerr << "something wrong has happened! ;)" << '\n';
			std::cerr << e.what() << "\n";
		}
	} catch (std::exception e) {
		std::cout << e.what() << std::endl;
	}
	return 0;
}
