

#include <iostream>
#include "thread"
#include <cpprest/http_listener.h>
#include <cpprest/http_client.h>
#include "controller/MasterController.h"
#include "Constants/CLIENT_DETAILS.h"

int main() {
    std::shared_ptr<services::LogManager> logManager = std::make_shared<services::LogManager>();
    std::shared_ptr<services::NetworkQueueManager> networkQueueManager = std::make_shared<services::NetworkQueueManager>(
            logManager);
    auto *workQueueManager = new services::WorkQueueManager(
            logManager, networkQueueManager);

    try{
        MasterController controller = MasterController(logManager, workQueueManager);
        web::http::client::http_client_config cfg;
        cfg.guarantee_order();
        web::http::experimental::listener::http_listener listener(
                "http://" + std::string(CONTROLLER_HOSTNAME) + ":" + std::to_string(CONTROLLER_REST_PORT) + "/controller");
        listener.support(web::http::methods::GET,
                         std::bind(&MasterController::handle_get, &controller, std::placeholders::_1));
        listener.support(web::http::methods::POST,
                         std::bind(&MasterController::handle_post, &controller, std::placeholders::_1));

        listener.open().wait();
        std::cout << "Listening for requests at: " << listener.uri().to_string() << std::endl;


        auto work_thread = std::thread(services::WorkQueueManager::main_loop, workQueueManager);
        work_thread.detach();
        auto network_thread = std::thread(services::NetworkQueueManager::initNetworkCommLoop, networkQueueManager);
        network_thread.detach();

        volatile bool hero = true;

        while(true){
            hero;
        }
    }
    catch (std::exception &e) {
        std::cerr << "something wrong has happened! ;)" << '\n';
        std::cerr << e.what() << "\n";
    }
    return 0;
}
