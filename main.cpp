

#include <iostream>
#include "utils/InterruptUtils.h"
#include "controller/MasterController.h"
#include "utils/RuntimeUtils.h"

using namespace utils;
using namespace controller;

int main() {
//    InterruptUtils::hookSIGINT();
    MasterController controller = MasterController();
//    server.setEndpoint("http://host_auto_ip4:6502/");

    web::http::experimental::listener::http_listener listener("http://host_auto_ip4:10000/");
    listener.support(web::http::methods::GET, std::bind(&MasterController::handleGet, &controller, std::placeholders::_1));
    listener.support(web::http::methods::POST, std::bind(&MasterController::handlePost, &controller, std::placeholders::_1));
    try {
        // wait for server initialization...
//        server.accept().wait();
//        std::cout << "Controller now listening for requests at: " << server.endpoint() << '\n';
        listener.open().wait();
        std::cout << "Listening for requests at: " << listener.uri().to_string() << std::endl;
        while (true);
//        InterruptUtils::waitForUserInterrupt();

//        server.shutdown().wait();
    }
    catch(std::exception & e) {
        std::cerr << "something wrong has happened! ;)" << '\n';
    }
    catch(...) {
//        RuntimeUtils::printStackTrace();
    }
    return 0;
}
