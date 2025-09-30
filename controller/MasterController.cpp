//
// Created by jamiec on 9/22/22.
//

#include "MasterController.h"
#include "../Constants/ControllerRequestPaths.h"
#include "../model/data_models/WorkItems/ProcessingItem/HighProcessingItem/HighProcessingItem.h"
#include "../model/data_models/WorkItems/BandwidthUpdate/BandwidthUpdateItem.h"
#include "../model/data_models/WorkItems/StateUpdate/StateUpdate.h"
#include <thread>
#include <string>
#include <utility>
#include "../Constants/CLIENT_DETAILS.h"
#include "../utils/IPerfTest/IPerfTest.h"
#include "../model/data_models/WorkItems/ProcessingItem/LowProcessingItem/LowProcessingItem.h"


using namespace web;
using namespace http;

void MasterController::handle_get(const http_request &message) {
    auto path = std::string(message.relative_uri().path());
    auto body = message.extract_json().get();
    if (!path.empty()) {
        json::value response;
        if (path == "/fetch_devices") {
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

void MasterController::deadline_violation(std::string message_remote_address, std::string dnn_id) {
    std::string host = std::move(message_remote_address);

    std::shared_ptr<std::vector<std::string>> hostList = std::make_shared<std::vector<std::string>>(
            std::initializer_list<std::string>{host});

    enums::request_type requestType = enums::request_type::state_update;

    web::json::value log;
    log["dnn_id"] = web::json::value::string(dnn_id);

    MasterController::logManager->add_log(enums::LogTypeEnum::VIOLATED_DEADLINE_REQUEST, log);

    auto stateUpdateItem = std::make_shared<model::StateUpdate>(hostList, requestType, std::chrono::system_clock::now(),
                                                                dnn_id);
    stateUpdateItem->setSuccess(false);

    workQueueManager->add_task(std::static_pointer_cast<model::WorkItem>(stateUpdateItem));
}

void MasterController::high_offload_request(int task_count, uint64_t deadline_ms, const std::string& message_remote_address,
                                            std::string dnn_id) {
    enums::request_type requestType = enums::request_type::high_complexity;

    std::chrono::time_point<std::chrono::system_clock> deadline{
            std::chrono::milliseconds(deadline_ms)};

    std::shared_ptr<std::vector<std::string>> hostList = std::make_shared<std::vector<std::string>>(
            std::initializer_list<std::string>{message_remote_address});

    dnn_id = message_remote_address + "_" + dnn_id;

    std::vector<std::string> dnn_ids;

    for (int i = 0; i < task_count; i++) {
        dnn_ids.push_back(dnn_id + "_" + std::to_string(i));
    }

    std::shared_ptr<model::HighProcessingItem> highProcessingItem = std::make_shared<model::HighProcessingItem>(
            hostList,
            requestType,
            deadline,
            dnn_id, dnn_ids);
    workQueueManager->add_task(std::static_pointer_cast<model::WorkItem>(highProcessingItem));

    web::json::value log;
    log["dnn_id"] = web::json::value::string(dnn_id);
    log["source_host"] = web::json::value::string(message_remote_address);
    log["deadline"] = web::json::value::number(deadline_ms);
    log["task_count"] = web::json::value::number(task_count);
    MasterController::logManager->add_log(enums::LogTypeEnum::HIGH_COMP_REQUEST, log);
};

void
MasterController::low_offload_request(uint64_t deadline_ms, std::string dnn_id, std::string message_remote_address) {
    enums::request_type requestType = enums::request_type::low_complexity;

    std::chrono::time_point<std::chrono::system_clock> deadline{
            std::chrono::milliseconds(deadline_ms)};

    std::string sourceDevice = message_remote_address;
    dnn_id = sourceDevice + "_" + dnn_id;

    std::shared_ptr<std::vector<std::string>> hostList = std::make_shared<std::vector<std::string>>(
            std::initializer_list<std::string>{sourceDevice});

    std::shared_ptr<model::LowProcessingItem> lowProcessingItem = std::make_shared<model::LowProcessingItem>(
            hostList, requestType, deadline,
            std::make_pair(dnn_id, sourceDevice));

    web::json::value log;
    log["dnn_id"] = web::json::value::string(dnn_id);
    log["source_host"] = web::json::value::string(message_remote_address);
    log["deadline"] = web::json::value::number(deadline_ms);
    MasterController::logManager->add_log(enums::LogTypeEnum::LOW_COMP_REQUEST, log);
    MasterController::workQueueManager->add_task(lowProcessingItem);
};

void
MasterController::state_update(std::string message_remote_address, std::uint64_t finish_time_ms, std::string dnn_id) {
    std::string host = std::move(message_remote_address);

    std::shared_ptr<std::vector<std::string>> hostList = std::make_shared<std::vector<std::string>>(
            std::initializer_list<std::string>{host});

    enums::request_type requestType = enums::request_type::state_update;

    std::chrono::time_point<std::chrono::system_clock> finish_time = std::chrono::time_point<std::chrono::system_clock>(
            std::chrono::milliseconds{finish_time_ms});

    web::json::value log;
    log["dnn_id"] = web::json::value::string(dnn_id);
    log["finish_time"] = web::json::value::number(finish_time_ms);

    MasterController::logManager->add_log(enums::LogTypeEnum::STATE_UPDATE_REQUEST, log);

    auto stateUpdateItem = std::make_shared<model::StateUpdate>(hostList, requestType, finish_time, dnn_id);

    workQueueManager->add_task(std::static_pointer_cast<model::WorkItem>(stateUpdateItem));
}

void MasterController::register_device(const std::string &message_remote_address) {
    dev_list->register_device(message_remote_address);
    std::shared_ptr<model::ComputationDevice> computationDevice = std::make_shared<model::ComputationDevice>(
            PER_DEVICE_CORES, message_remote_address);
    workQueueManager->network->devices[message_remote_address] = computationDevice;
    workQueueManager->networkQueueManager->hosts.push_back(message_remote_address);

    web::json::value log;
    log["host"] = web::json::value::string(message_remote_address);
    MasterController::logManager->add_log(enums::LogTypeEnum::DEVICE_REGISTER, log);
    if (dev_list->get_devices().size() == CLIENT_COUNT) {
        std::pair<double, double> bits_per_second = utils::iPerfTest(dev_list->get_devices());
        workQueueManager->setAverageBitsPerSecond(bits_per_second.first);
        workQueueManager->setJitter(bits_per_second.second);

        web::json::value bits_log;
        bits_log["bits_per_second"] = web::json::value::number(workQueueManager->getAverageBitsPerSecond());
        bits_log["jitter"] = web::json::value::number(workQueueManager->getJitter());
        MasterController::logManager->add_log(enums::LogTypeEnum::IPERF_RESULTS, bits_log);
        /* Need to communicate outbound here */
        std::shared_ptr<model::BaseNetworkCommsModel> baseNetworkCommsModel = std::make_shared<model::BaseNetworkCommsModel>(
                enums::network_comms_types::initial_experiment_start, std::chrono::system_clock::now());
        workQueueManager->networkQueueManager->addTask(baseNetworkCommsModel);
    }
};

void MasterController::bandwidth_update(std::string host, std::vector<double> bits_per_second){

		std::shared_ptr<std::vector<std::string>> hostList = std::make_shared<std::vector<std::string>>(
				std::initializer_list<std::string>{host});

		enums::request_type requestType = enums::request_type::bandwidth_update;
		web::json::value log;

		web::json::value json_array = web::json::value::array(static_cast<unsigned int>(bits_per_second.size()));
		for (size_t i = 0; i < bits_per_second.size(); ++i)
			json_array[i] = json::value::number(bits_per_second[i]);

		log["bits_per_second"] = json_array;
		log["source_device"] = web::json::value::string(host);
		log["time"] = web::json::value::number(std::chrono::system_clock::now().time_since_epoch().count());
		MasterController::logManager->add_log(enums::LogTypeEnum::BANDWIDTH_UPDATE_LOG, log);

		auto bandwidth_update_item = std::make_shared<model::BandwidthUpdateItem>(requestType, bits_per_second);
		workQueueManager->add_task(std::static_pointer_cast<model::WorkItem>(bandwidth_update_item));
}

void MasterController::handle_post(const http_request &message) {
    auto path = std::string(message.relative_uri().path());
    auto body = message.extract_json().get();
    std::cout << "INBOUND REQ: " << path << std::endl;
    if (path == "/deadline_violation")
        std::cout << "";
    try {
        if (!path.empty()) {
            std::cout << "REQUEST_RECEIVED" << std::endl;
            if (path == LOG_RESULT) {
                std::string result = MasterController::logManager->write_log();
                message.reply(status_codes::OK, result).wait();
            } else if (path == HIGH_OFFLOAD_REQUEST) {
                message.reply(status_codes::OK);

                MasterController::high_offload_request(body["task_count"].as_integer(),
                                                       body["deadline"].as_number().to_uint64(),
                                                       message.remote_address(), body["dnn_id"].as_string());

            } else if (path == LOW_OFFLOAD_REQUEST) {
                http_response response;
                response.set_status_code(status_codes::Created);
                message.reply(response);

                low_offload_request(body["deadline"].as_number().to_uint64(), body["dnn_id"].as_string(),
                                    message.remote_address());

            } else if (path == DEVICE_REGISTER_REQUEST) {
                web::json::value response;
                response["host_name"] = json::value::string(message.remote_address());
                message.reply(status_codes::Created, response);

                register_device(message.remote_address());

            } else if (path == STATE_UPDATE) {
                message.reply(status_codes::OK);
                state_update(message.remote_address(), body["finish_time"].as_number().to_uint64(),
                             body["dnn_id"].as_string());

            } else if (path == DEADLINE_VIOLATED) {
                message.reply(status_codes::OK);
                deadline_violation(message.remote_address(), body["dnn_id"].as_string());

            }
			else if (path == BANDWIDTH_UPDATE){
				message.reply(status_codes::OK);
				std::vector<double> bits_per_second_vect = {} ;
				auto json_bps_arr = body["bits_per_second"].as_array();
				for (auto& element : json_bps_arr)
					bits_per_second_vect.push_back(element.as_number().to_double());

				bandwidth_update(message.remote_address(), bits_per_second_vect);
			}
        } else {
            message.reply(status_codes::NotFound);
        }
    }
    catch (std::exception &e) {
        std::cerr << "CONTROLLER: something wrong has happened! ;)" << '\n';
        std::cerr << e.what() << "\n";
    }
}

MasterController::MasterController(std::shared_ptr<services::LogManager> ptr,
                                   services::WorkQueueManager *sharedPtr) {
    MasterController::logManager = ptr;
    MasterController::workQueueManager = sharedPtr;
}
