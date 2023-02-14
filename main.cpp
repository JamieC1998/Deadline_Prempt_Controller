

#include <iostream>
#include "thread"
#include <cpprest/http_listener.h>
#include "controller/MasterController.h"
#include "Constants/CLIENT_DETAILS.h"

int main() {
    std::shared_ptr<services::LogManager> logManager = std::make_shared<services::LogManager>();
    std::shared_ptr<services::NetworkQueueManager> networkQueueManager = std::make_shared<services::NetworkQueueManager>(
            logManager);
    std::shared_ptr<services::WorkQueueManager> workQueueManager = std::make_shared<services::WorkQueueManager>(
            logManager, networkQueueManager);

    MasterController controller = MasterController(logManager, workQueueManager);

    web::http::experimental::listener::http_listener listener("http://localhost:" + std::to_string(CONTROLLER_REST_PORT) + "/");
    listener.support(web::http::methods::GET,
                     std::bind(&MasterController::handle_get, &controller, std::placeholders::_1));
    listener.support(web::http::methods::POST,
                     std::bind(&MasterController::handle_post, &controller, std::placeholders::_1));

    try {
        listener.open().wait();
        std::cout << "Listening for requests at: " << listener.uri().to_string() << std::endl;

        auto work_thread = std::thread(services::WorkQueueManager::main_loop, workQueueManager);
        auto network_thread = std::thread(services::NetworkQueueManager::initNetworkCommLoop, networkQueueManager);
        while (true);
    }
    catch (std::exception &e) {
        std::cerr << "something wrong has happened! ;)" << '\n';
    }
    return 0;
}
