

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

int main() {

	auto jsonObj = services::read_experiment_log(PROFILE_INPUT_FILE);
	auto [state_u_list, simEventList] = services::parse_experiment_log(jsonObj);

	try {
		model::IsProfilingSingleton::setIsProfiling(true);
		auto profile_map = services::profiler_event_loop(state_u_list, simEventList);
		auto load_transformation = services::profile_data_transform(profile_map);

		services::writeTransformedLoadResults(load_transformation);

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
		}
		catch (std::exception &e) {
			std::cerr << "something wrong has happened! ;)" << '\n';
			std::cerr << e.what() << "\n";
		}
	}
	catch (std::exception e) {
		std::cout << e.what() << std::endl;
	}
	return 0;
}
