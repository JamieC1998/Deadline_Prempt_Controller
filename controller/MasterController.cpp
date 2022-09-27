//
// Created by jamiec on 9/22/22.
//

#include "MasterController.h"
#include "../Constants/RequestPaths.h"
#include "../model/data_models/WorkItems/ProcessingItem/ProcessingItem.h"
#include "../model/data_models/WorkItems/DAGDisruptItem/DAGDisruption.h"
//#include "../model/ProcessOffloadRequest/ProcessOffloadRequest.h"

using namespace web;
using namespace http;
using namespace controller;

void MasterController::initRestOpHandlers() {
    _listener.support(methods::GET, std::bind(&MasterController::handleGet, this, std::placeholders::_1));
    _listener.support(methods::POST, std::bind(&MasterController::handlePost, this, std::placeholders::_1));
    _listener.support(methods::DEL, std::bind(&MasterController::handleDelete, this, std::placeholders::_1));
}

void MasterController::handleGet(http_request message) {
    auto path = requestPath(message);
    auto body = message.extract_json().get();
    if (!path.empty()) {
        json::value response;
        if (path[0] == "fetch_devices") {
            std::vector<json::value> val_list;
            auto list = dev_list->get_devices();

            for (const auto &item: list)
                val_list.push_back(json::value::string(item));
            response["list"] = json::value::array(val_list);
            message.reply(status_codes::Accepted, response);
        } else
            message.reply(status_codes::NotFound);
    } else {
        message.reply(status_codes::NotFound);
    }
    message.reply(status_codes::NotFound);
}

void MasterController::handlePost(http_request message) {
    auto path = requestPath(message);
    auto body = message.extract_json().get();
    if (!path.empty()) {
        json::value response;
        if (path[0] == OFFLOAD_REQUEST) {
            message.reply(status_codes::OK);

            workQueueManager->add_task(reinterpret_cast<model::WorkItem *>(new model::ProcessingItem(
                    static_cast<enums::request_type>(body["offload_type"].as_integer()),
                    std::make_shared<std::vector<std::string>>(
                            std::initializer_list<std::string>{message.remote_address()}),
                    std::initializer_list<std::map<std::string, std::shared_ptr<model::BaseDNNModel>>::value_type>{
                            {message.remote_address(),
                             std::make_shared<model::BaseDNNModel>()}})));

        } else if (path[0] == DEVICE_REGISTER_REQUEST) {
            dev_list->register_device(message.remote_address());
            message.reply(status_codes::Created);
        } else if (path[0] == DAG_DISRUPTION) {
            message.reply(status_codes::OK);
            workQueueManager->add_task(reinterpret_cast<model::WorkItem *>(new model::DAGDisruption(
                    static_cast<enums::request_type>(body["offload_type"].as_integer()),
                    std::make_shared<std::vector<std::string>>(
                            std::initializer_list<std::string>{message.remote_address()}),
                    body["partition_dnn_id"].as_integer(), body["block_id"].as_integer(),
                    body["partition_id"].as_integer(), body["layer_id"].as_integer())));
        }
    } else {
        message.reply(status_codes::NotFound);
    }
}

void MasterController::handleDelete(http_request message) {
    message.reply(status_codes::NotFound);
}

json::value MasterController::responseNotImpl(const http::method &method) {
    auto response = json::value::object();
    response["serviceName"] = json::value::string("C++ Mircroservice Sample");
    response["http_method"] = json::value::string(method);
    return response;
}

MasterController::MasterController() {
    workQueueManager = std::make_shared<services::WorkQueueManager>();
}
