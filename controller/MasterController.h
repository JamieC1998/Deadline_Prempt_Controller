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

    void high_offload_request(int task_count, uint64_t deadline_ms, const std::string& message_remote_address, std::string dnn_id);

    void low_offload_request(uint64_t deadline_ms, std::string dnn_id, std::string message_remote_address);

    void register_device(const std::string& message_remote_address);

    void state_update(std::string message_remote_address, std::uint64_t finish_time_ms, std::string dnn_id);

    void deadline_violation(std::string message_remote_address, std::string dnn_id);

	void bandwidth_update(std::string host, std::vector<double> bits_per_second);

private:
    std::shared_ptr<services::RegisterDevices> dev_list = std::make_shared<services::RegisterDevices>();
    services::WorkQueueManager *workQueueManager;
    std::shared_ptr<services::LogManager> logManager;

};


#endif //CONTROLLER_MASTERCONTROLLER_H
