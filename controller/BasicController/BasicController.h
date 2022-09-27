//
// Created by jamiec on 9/22/22.
//

#ifndef CONTROLLER_BASICCONTROLLER_H
#define CONTROLLER_BASICCONTROLLER_H

#include <string>
#include <cpprest/http_listener.h>
#include <pplx/pplxtasks.h>

namespace controller {
    class BasicController {
    protected:
        web::http::experimental::listener::http_listener _listener;

    public:
        BasicController();
        ~BasicController();

        void setEndpoint(const std::string &endpoint);

        std::string endpoint() const;

        pplx::task<void> accept();
        pplx::task<void> shutdown();

        virtual void initRestOpHandlers() {

        }

        std::vector<utility::string_t> requestPath(const web::http::http_request &message);
    };
}

#endif //CONTROLLER_BASICCONTROLLER_H
