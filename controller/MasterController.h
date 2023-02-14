//
// Created by jamiec on 9/22/22.
//

#ifndef CONTROLLER_MASTERCONTROLLER_H
#define CONTROLLER_MASTERCONTROLLER_H

#include <thread>
#include "BasicController/BasicController.h"
#include "../Services/DeviceRegister/RegisterDevices.h"
#include "../Services/WorkQueueManager/WorkQueueManager.h"
#include "../Services/LOG_MANAGER/LogManager.h"

namespace controller {
//    class MasterController : public BasicController {
    class MasterController {

    public:
        MasterController();

        void handleGet(web::http::http_request message);

        void handlePost(web::http::http_request message);

//        void handleDelete(web::http::http_request message);
//
//        void initRestOpHandlers();

    private:
        std::shared_ptr<services::RegisterDevices> dev_list = std::make_shared<services::RegisterDevices>();
//        static web::json::value responseNotImpl(const web::http::method & method);
        std::shared_ptr<services::WorkQueueManager> workQueueManager;
        std::shared_ptr<services::LogManager> logManager;
        std::thread workQueueThread;

    };
}

#endif //CONTROLLER_MASTERCONTROLLER_H
