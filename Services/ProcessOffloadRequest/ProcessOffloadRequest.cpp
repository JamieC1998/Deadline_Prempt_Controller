//
// Created by jamiec on 9/22/22.
//

#include "ProcessOffloadRequest.h"

namespace model {
    json::value ProcessOffloadRequest::process_request() {
        auto response = json::value::object();

        response["model"] = json::value::string("partitioned_successfully!");
        return response;
    }
} // model