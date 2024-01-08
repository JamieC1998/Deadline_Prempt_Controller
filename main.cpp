

#include <iostream>
#include "thread"
#include <cpprest/http_listener.h>
#include <cpprest/http_client.h>
#include "controller/MasterController.h"
#include "Constants/CLIENT_DETAILS.h"

void unhandledExceptionCallback() {
    try {
        throw; // Re-throw the exception to obtain exception information
    } catch (const std::exception &e) {
        std::cout << "Unhandled exception caught: " << e.what() << std::endl;
        std::cout << e.what() << std::endl;
    } catch (const web::http::http_exception &e) {
        std::cout << "Unhandled unknown exception caught" << std::endl;
        std::cout << e.what() << std::endl;
    }

    std::abort(); // Terminate the program
}

int main() {
    std::set_terminate(unhandledExceptionCallback);

    try {
        std::shared_ptr<services::LogManager> logManager = std::make_shared<services::LogManager>();

        auto *workQueueManager = new services::WorkQueueManager(
                logManager);

        MasterController controller = MasterController(logManager, workQueueManager);
        web::http::client::http_client_config cfg;
        cfg.guarantee_order();
        web::http::experimental::listener::http_listener listener(
                "http://" + std::string(CONTROLLER_HOSTNAME) + ":" + std::to_string(CONTROLLER_REST_PORT) +
                "/controller");
        listener.support(web::http::methods::GET,
                         std::bind(&MasterController::handle_get, &controller, std::placeholders::_1));
        listener.support(web::http::methods::POST,
                         std::bind(&MasterController::handle_post, &controller, std::placeholders::_1));

        listener.open().wait();
        std::cout << "Listening for requests at: " << listener.uri().to_string() << std::endl;

        auto work_thread = std::thread(services::WorkQueueManager::main_loop, workQueueManager);
        work_thread.detach();

        while (true);
    }
    catch (std::exception &e) {
        std::cout << "something wrong has happened! ;)" << '\n';
        std::cout << e.what() << "\n";
    }
    return 0;
}
