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
                     services::WorkQueueManager *sharedPtr);

    void handle_get(const web::http::http_request &message);

    void handle_post(const web::http::http_request &message);

private:
    std::shared_ptr<services::RegisterDevices> dev_list = std::make_shared<services::RegisterDevices>();
    services::WorkQueueManager *workQueueManager;
    std::shared_ptr<services::LogManager> logManager;

    std::vector<std::string> high_allocate_req_id;
    std::vector<std::string> high_work_req_id;
    std::vector<std::string> low_allocate_req_id;
    std::vector<std::string> state_update_req_id;
    std::vector<std::string> deadline_violated_req_id;

};

static void initialise_experiment(std::vector<std::string> hosts, std::shared_ptr<services::LogManager> logManager);

#endif //CONTROLLER_MASTERCONTROLLER_H
