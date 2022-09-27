

#include <iostream>
#include "utils/InterruptUtils.h"
#include "controller/MasterController.h"
#include "utils/RuntimeUtils.h"

using namespace utils;
using namespace controller;

int main() {
    InterruptUtils::hookSIGINT();
    MasterController server;
    server.setEndpoint("http://host_auto_ip4:6502/controller");

    try {
        // wait for server initialization...
        server.accept().wait();
        std::cout << "Controller now listening for requests at: " << server.endpoint() << '\n';

        InterruptUtils::waitForUserInterrupt();

        server.shutdown().wait();
    }
    catch(std::exception & e) {
        std::cerr << "something wrong has happened! ;)" << '\n';
    }
    catch(...) {
        RuntimeUtils::printStackTrace();
    }
    return 0;
}
