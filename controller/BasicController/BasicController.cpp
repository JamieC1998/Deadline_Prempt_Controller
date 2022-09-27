//
// Created by jamiec on 9/22/22.
//
#include "BasicController.h"
#include "../../utils/NetworkUtils/NetUtils.h"

namespace controller {
    BasicController::BasicController(){}
    BasicController::~BasicController() {}

    void BasicController::setEndpoint(const std::string & value) {
        web::uri endpointURI(value);
        web::uri_builder endpointBuilder;

        endpointBuilder.set_scheme(endpointURI.scheme());
        if (endpointURI.host() == "host_auto_ip4") {
            endpointBuilder.set_host(utils::NetUtils::hostIP4());
        }
        else if (endpointURI.host() == "host_auto_ip6") {
            endpointBuilder.set_host(utils::NetUtils::hostIP6());
        }
        endpointBuilder.set_port(endpointURI.port());
        endpointBuilder.set_path(endpointURI.path());

        _listener = web::http::experimental::listener::http_listener(endpointBuilder.to_uri());
    }

    std::string BasicController::endpoint() const {
        return _listener.uri().to_string();
    }

    pplx::task<void> BasicController::accept() {
        initRestOpHandlers();
        return _listener.open();
    }

    pplx::task<void> BasicController::shutdown() {
        return _listener.close();
    }

    std::vector<utility::string_t> BasicController::requestPath(const web::http::http_request & message) {
        auto relativePath = web::uri::decode(message.relative_uri().path());
        return web::uri::split_path(relativePath);
    }
}

