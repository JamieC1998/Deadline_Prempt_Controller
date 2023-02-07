//
// Created by jamiec on 9/22/22.
//

#include "MasterController.h"
#include "../Constants/ControllerRequestPaths.h"
#include "../model/data_models/WorkItems/ProcessingItem/HighProcessingItem/HighProcessingItem.h"
#include "../model/data_models/WorkItems/DAGDisruptItem/DAGDisruption.h"
#include "../model/data_models/WorkItems/StateUpdate/StateUpdate.h"
#include "../utils/UtilFunctions/UtilFunctions.h"
#include <iperf_api.h>
#include "../Constants/CLIENT_DETAILS.h"
#include "../utils/IPerfTest/IPerfTest.h"
#include "../model/data_models/WorkItems/ProcessingItem/LowProcessingItem/LowProcessingItem.h"


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
        if (path[0] == HIGH_OFFLOAD_REQUEST) {
            message.reply(status_codes::OK);

            enums::request_type requestType = enums::request_type::high_complexity;

            std::chrono::time_point<std::chrono::system_clock> deadline{
                    std::chrono::milliseconds(body["deadline"].as_integer())};

            std::shared_ptr<std::vector<std::string>> hostList = std::make_shared<std::vector<std::string>>(
                    std::initializer_list<std::string>{message.remote_address()});

            std::string dnn_id = body["dnn_id"].as_string();

            std::shared_ptr<model::HighProcessingItem> highProcessingItem = std::make_shared<model::HighProcessingItem>(
                    hostList,
                    requestType,
                    deadline,
                    dnn_id);

            workQueueManager->add_task(std::static_pointer_cast<model::WorkItem>(highProcessingItem));

        } else if (path[0] == LOW_OFFLOAD_REQUEST) {
            message.reply(status_codes::OK);

            enums::request_type requestType = enums::request_type::low_complexity;

            std::chrono::time_point<std::chrono::system_clock> deadline{
                    std::chrono::milliseconds(body["deadline"].as_integer())};

            std::string dnn_id = body["dnn_id"].as_string();

            std::string sourceDevice = message.remote_address();

            std::shared_ptr<std::vector<std::string>> hostList = std::make_shared<std::vector<std::string>>(
                    std::initializer_list<std::string>{sourceDevice});

            std::shared_ptr<model::LowProcessingItem> lowProcessingItem = std::make_shared<model::LowProcessingItem>(
                    hostList, requestType, deadline,
                    std::make_pair(dnn_id, sourceDevice));

        } else if (path[0] == DEVICE_REGISTER_REQUEST) {
            dev_list->register_device(message.remote_address());
            message.reply(status_codes::Created);
            if (dev_list->get_devices().size() == CLIENT_COUNT) {
                std::pair<double, double> bits_per_second = utils::iPerfTest(dev_list->get_devices());
                workQueueManager->setAverageBitsPerSecond(bits_per_second.first);
                workQueueManager->setJitter(bits_per_second.second);
            }

        } else if (path[0] == DAG_DISRUPTION_PATH) {
            message.reply(status_codes::OK);

            enums::request_type requestType = enums::request_type::dag_disruption;
            std::string host = message.remote_address();

            std::string dnn_id = body["dnn_id"].as_string();
            std::string convidx = body["convidx"].as_string();
            int partition_id = body["partition_id"].as_integer();

            std::shared_ptr<std::vector<std::string>> hostList = std::make_shared<std::vector<std::string>>(
                    std::initializer_list<std::string>{host});


            std::chrono::time_point<std::chrono::system_clock> finish_time{
                    std::chrono::milliseconds(body["finish_time"].as_integer())};

            std::shared_ptr<model::DAGDisruption> dagDisruption = std::make_shared<model::DAGDisruption>(hostList,
                                                                                                         requestType,
                                                                                                         dnn_id,
                                                                                                         convidx,
                                                                                                         partition_id,
                                                                                                         finish_time);

            workQueueManager->add_task(std::static_pointer_cast<model::WorkItem>(dagDisruption));
        } else if (path[0] == STATE_UPDATE) {
            message.reply(status_codes::OK);

            std::string host = message.remote_address();

            std::shared_ptr<std::vector<std::string>> hostList = std::make_shared<std::vector<std::string>>(
                    std::initializer_list<std::string>{host});

            enums::request_type requestType = enums::request_type::state_update;

            std::map<int, std::chrono::time_point<std::chrono::system_clock>> finish_map;

            auto finish_list = body["finish_times"].as_array();

            for(auto finish_item: finish_list){
                auto finish_object = finish_item.as_object();
                int partition_id = finish_object["partition_id"].as_integer();
                uint64_t finish_time = finish_object["finish_time"].as_integer();

                finish_map[partition_id] = std::chrono::time_point<std::chrono::system_clock>(std::chrono::milliseconds {finish_time});
            }

            std::string convidx = body["conv_idx"].as_string();
            std::string dnn_id = body["dnn_id"].as_string();

            auto stateUpdateItem = std::make_shared<model::StateUpdate>(hostList, requestType, finish_map, convidx, dnn_id);

            workQueueManager->add_task(std::static_pointer_cast<model::WorkItem>(stateUpdateItem));
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
