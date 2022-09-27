//
// Created by jamiec on 9/22/22.
//

#ifndef CONTROLLER_PROCESSOFFLOADREQUEST_H
#define CONTROLLER_PROCESSOFFLOADREQUEST_H
#include <cpprest/http_listener.h>

using namespace web;

namespace model {

    class ProcessOffloadRequest {
    public:
        static json::value process_request();
    };

} // model

#endif //CONTROLLER_PROCESSOFFLOADREQUEST_H
