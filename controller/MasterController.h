//
// Created by jamiec on 9/22/22.
//

#ifndef CONTROLLER_MASTERCONTROLLER_H
#define CONTROLLER_MASTERCONTROLLER_H

#include <cpprest/http_listener.h>
#include <cpprest/json.h>
#include "../Services/DeviceRegister/RegisterDevices.h"
#include "../Services/WorkQueueManager/WorkQueueManager.h"

class MasterController {

public:
    MasterController(std::shared_ptr<services::LogManager> ptr,
                     std::shared_ptr<services::WorkQueueManager> sharedPtr);

    void handle_get(web::http::http_request message);

    void handle_post(web::http::http_request message);

private:
    std::shared_ptr<services::RegisterDevices> dev_list = std::make_shared<services::RegisterDevices>();
    std::shared_ptr<services::WorkQueueManager> workQueueManager;
    std::shared_ptr<services::LogManager> logManager;

};


#endif //CONTROLLER_MASTERCONTROLLER_H
