//
// Created by jamiec on 9/22/22.
//

#include "MasterController.h"
#include "../Constants/ControllerRequestPaths.h"
#include "../model/data_models/WorkItems/ProcessingItem/HighProcessingItem/HighProcessingItem.h"
#include "../model/data_models/WorkItems/StateUpdate/StateUpdate.h"
#include "../utils/UtilFunctions/UtilFunctions.h"
#include <iperf_api.h>
#include "cpprest/http_client.h"
#include <thread>
#include "../Constants/CLIENT_DETAILS.h"
#include "../utils/IPerfTest/IPerfTest.h"
#include "../model/data_models/WorkItems/ProcessingItem/LowProcessingItem/LowProcessingItem.h"
#include "../model/data_models/WorkItems/WorkRequest/WorkRequest.h"
#include "../model/data_models/WorkItems/HaltWorkItem/HaltWorkItem.h"


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

void MasterController::handle_post(const http_request &message) {
    auto path = std::string(message.relative_uri().path());
    auto body = message.extract_json().get();

    std::cout << "INBOUND REQ: " << path << std::endl;

    try {
        if (!path.empty()) {
            message.reply(status_codes::OK);

            std::cout << "REQUEST_RECEIVED" << std::endl;
            if (path == LOG_RESULT) {
                std::string result = MasterController::logManager->write_log();
            } else if (path == POST_HALT_TASK) {
                auto source_host = message.remote_address();
                auto dnn_id = body["dnn_id"].as_string();

                auto haltItem = std::make_shared<model::HaltWorkItem>(enums::request_type::halt_req, source_host,
                                                                      dnn_id);
                workQueueManager->add_task(haltItem);
            } else if (path == POST_LOW_TASK) {
                auto source_host = message.remote_address();
                auto dnn_id = body["dnn_id"].as_string();
                uint64_t start_time = body["start_time"].as_integer();
                uint64_t finish_time = body["finish_time"].as_integer();
                auto request_id = body["request_id"].as_string();
                auto invoke_preemption = body["invoke_preemption"].as_bool();


                // Use std::find to check if the string is in the vector
                auto it = std::find(MasterController::low_allocate_req_id.begin(),
                                    MasterController::low_allocate_req_id.end(),
                                    message.remote_address() + "_" + request_id);

                if (it == MasterController::low_allocate_req_id.end()) {


                    std::shared_ptr<std::vector<std::string>> hostList = std::make_shared<std::vector<std::string>>(
                            std::initializer_list<std::string>{source_host});

                    enums::request_type requestType = enums::request_type::low_complexity;

                    std::shared_ptr<model::LowProcessingItem> lowProcessingItem = std::make_shared<model::LowProcessingItem>(
                            hostList, requestType, source_host, dnn_id,
                            std::chrono::time_point<std::chrono::system_clock>{
                                    std::chrono::milliseconds(start_time)},
                            std::chrono::time_point<std::chrono::system_clock>{
                                    std::chrono::milliseconds(finish_time)}, invoke_preemption);

                    web::json::value log;
                    log["dnn_id"] = web::json::value::string(dnn_id);
                    log["source_host"] = web::json::value::string(message.remote_address());
                    log["start_time"] = web::json::value::number(start_time);
                    log["finish_time"] = web::json::value::number(finish_time);
                    MasterController::logManager->add_log(enums::LogTypeEnum::LOW_COMP_REQUEST, log);

                    MasterController::workQueueManager->add_task(lowProcessingItem);
                    MasterController::low_allocate_req_id.push_back(message.remote_address() + "_" + request_id);
                } else {
                    std::cout << "DUPLICATE LOW OFFLOAD REQUEST FROM: " << message.remote_address() << std::endl;
                }
            }
            else if (path == POST_LOW_COMP_FAIL){
                auto source_host = message.remote_address();
                auto dnn_id = body["dnn_id"].as_string();
                uint64_t time = body["time"].as_integer();

                std::chrono::time_point<std::chrono::system_clock> fail_time {std::chrono::milliseconds{time}};

                web::json::value log;
                log["dnn_id"] = web::json::value::string(dnn_id);
                log["source_host"] = web::json::value::string(message.remote_address());
                log["time"] = web::json::value::number(time);
                MasterController::logManager->add_log(enums::LogTypeEnum::LOW_COMP_ALLOCATION_FAIL, log);
            }
            else if (path == HIGH_WORK_REQUEST) {

                enums::request_type requestType = enums::request_type::work_request;


                std::string host = message.remote_address();
                std::shared_ptr<std::vector<std::string>> hostList = std::make_shared<std::vector<std::string>>(
                        std::initializer_list<std::string>{host});

                int request_counter = body["request_counter"].as_integer();
                auto request_id = body["request_id"].as_string();
                int capacity = body["capacity"].as_integer();

                // Use std::find to check if the string is in the vector
                auto it = std::find(MasterController::high_work_req_id.begin(),
                                    MasterController::high_work_req_id.end(),
                                    message.remote_address() + "_" + request_id);


                if (it == MasterController::high_work_req_id.end()) {

                    std::shared_ptr<model::WorkRequest> workRequestItem = std::make_shared<model::WorkRequest>(hostList,
                                                                                                               requestType,
                                                                                                               capacity,
                                                                                                               request_counter,
                                                                                                               std::chrono::system_clock::now());

                    web::json::value log;
                    log["source_host"] = web::json::value::string(message.remote_address());
                    log["request_counter"] = web::json::value::number(request_counter);

                    MasterController::logManager->add_log(enums::LogTypeEnum::WORK_REQUEST, log);

                    MasterController::workQueueManager->add_task(workRequestItem);
                    MasterController::high_work_req_id.push_back(message.remote_address() + "_" + request_id);
                } else {
                    std::cout << "DUPLICATE WORK REQUEST FROM: " << message.remote_address() << std::endl;
                }

            } else if (path == HIGH_OFFLOAD_REQUEST) {
                enums::request_type requestType = enums::request_type::high_complexity;
                uint64_t deadline_ms = body["deadline"].as_number().to_uint64();
                auto request_id = body["request_id"].as_string();
                int task_count = body["task_count"].as_integer();

                // Use std::find to check if the string is in the vector
                auto it = std::find(MasterController::high_allocate_req_id.begin(),
                                    MasterController::high_allocate_req_id.end(),
                                    message.remote_address() + "_" + request_id);


                if (it == MasterController::high_allocate_req_id.end()) {
                    std::chrono::time_point<std::chrono::system_clock> deadline{
                            std::chrono::milliseconds(deadline_ms)};

                    std::shared_ptr<std::vector<std::string>> hostList = std::make_shared<std::vector<std::string>>(
                            std::initializer_list<std::string>{message.remote_address()});

                    std::string dnn_id = body["dnn_id"].as_string();
                    dnn_id = message.remote_address() + "_" + dnn_id;

                    for (int i = 0; i < task_count; i++) {
                        std::shared_ptr<model::HighProcessingItem> highProcessingItem = std::make_shared<model::HighProcessingItem>(
                                hostList,
                                requestType,
                                deadline,
                                dnn_id + "_" + std::to_string(i));

                        auto id = highProcessingItem->getDnnId();

                        workQueueManager->add_task(std::static_pointer_cast<model::WorkItem>(highProcessingItem));
                    }


                    web::json::value log;
                    log["dnn_id"] = web::json::value::string(dnn_id);
                    log["source_host"] = web::json::value::string(message.remote_address());
                    log["deadline"] = web::json::value::number(body["deadline"].as_number().to_uint64());
                    log["task_count"] = web::json::value::number(task_count);
                    MasterController::logManager->add_log(enums::LogTypeEnum::HIGH_COMP_REQUEST, log);
                    MasterController::high_allocate_req_id.push_back(message.remote_address() + "_" + request_id);
                } else {
                    std::cout << "DUPLICATE HIGH OFFLOAD REQUEST FROM: " << message.remote_address() << std::endl;
                }
                return;

            } else if (path == DEVICE_REGISTER_REQUEST) {
                dev_list->register_device(message.remote_address());
                std::shared_ptr<model::ComputationDevice> computationDevice = std::make_shared<model::ComputationDevice>(
                        PER_DEVICE_CORES, message.remote_address());
                workQueueManager->network->devices[message.remote_address()] = computationDevice;
                web::json::value response;
                response["host_name"] = json::value::string(message.remote_address());

                web::json::value log;
                log["host"] = web::json::value::string(message.remote_address());
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
                    initialise_experiment(dev_list->get_devices(), MasterController::logManager, ((bits_per_second.first - bits_per_second.second) / 8) * 1000);
                }

            } else if (path == STATE_UPDATE) {
                std::string body_dump = body.serialize();
                std::cout << body_dump << std::endl;
                std::string host = message.remote_address();

                auto request_id = body["request_id"].as_string();


                // Use std::find to check if the string is in the vector
                auto it = std::find(MasterController::state_update_req_id.begin(),
                                    MasterController::state_update_req_id.end(),
                                    message.remote_address() + "_" + request_id);

                if (it == MasterController::state_update_req_id.end()) {

                    std::shared_ptr<std::vector<std::string>> hostList = std::make_shared<std::vector<std::string>>(
                            std::initializer_list<std::string>{host});

                    enums::request_type requestType = enums::request_type::state_update;

                    std::chrono::time_point<std::chrono::system_clock> finish_time = std::chrono::time_point<std::chrono::system_clock>(
                            std::chrono::milliseconds{body["finish_time"].as_number().to_uint64()});
                    std::string dnn_id = body["dnn_id"].as_string();

                    web::json::value log;
                    log["dnn_id"] = web::json::value::string(dnn_id);
                    log["finish_time"] = web::json::value::number(body["finish_time"].as_number().to_uint64());

                    MasterController::logManager->add_log(enums::LogTypeEnum::STATE_UPDATE_REQUEST, log);

                    auto stateUpdateItem = std::make_shared<model::StateUpdate>(hostList, requestType, finish_time,
                                                                                dnn_id);

                    workQueueManager->add_task(std::static_pointer_cast<model::WorkItem>(stateUpdateItem));
                    MasterController::state_update_req_id.push_back(message.remote_address() + "_" + request_id);
                } else {
                    std::cout << "DUPLICATE STATE UPDATE REQUEST FROM: " << message.remote_address() << std::endl;
                }
            } else if (path == DEADLINE_VIOLATED) {
                std::string body_dump = body.serialize();
                std::cout << body_dump << std::endl;
                std::string host = message.remote_address();

                auto request_id = body["request_id"].as_string();


                // Use std::find to check if the string is in the vector
                auto it = std::find(MasterController::deadline_violated_req_id.begin(),
                                    MasterController::deadline_violated_req_id.end(),
                                    message.remote_address() + "_" + request_id);

                if (it == MasterController::deadline_violated_req_id.end()) {

                    std::shared_ptr<std::vector<std::string>> hostList = std::make_shared<std::vector<std::string>>(
                            std::initializer_list<std::string>{host});

                    enums::request_type requestType = enums::request_type::state_update;

                    std::string dnn_id = body["dnn_id"].as_string();

                    web::json::value log;
                    log["dnn_id"] = web::json::value::string(dnn_id);
                    log["allocated_host"] = web::json::value::string(host);

                    MasterController::logManager->add_log(enums::LogTypeEnum::VIOLATED_DEADLINE_REQUEST, log);

                    auto stateUpdateItem = std::make_shared<model::StateUpdate>(hostList, requestType,
                                                                                std::chrono::system_clock::now(),
                                                                                dnn_id);
                    stateUpdateItem->setSuccess(false);

                    workQueueManager->add_task(std::static_pointer_cast<model::WorkItem>(stateUpdateItem));
                    MasterController::deadline_violated_req_id.push_back(message.remote_address() + "_" + request_id);
                } else {
                    std::cout << "DUPLICATE DEADLINE VIOLATION REQUEST FROM: " << message.remote_address() << std::endl;
                }
            }
        } else {
            message.reply(status_codes::NotFound);
        }
    }
    catch (std::exception &e) {
        message.reply(status_codes::NotFound);
        std::cout << "CONTROLLER: something wrong has happened! ;)" << '\n';
        std::cout << e.what() << "\n";
    }
    catch (const http::http_exception &e) {
        message.reply(status_codes::NotFound);
        std::cout << "CONTROLLER: something wrong has happened! ;)" << '\n';
        std::cout << e.what() << "\n";
    }
}

void initialise_experiment(std::vector<std::string> hosts, std::shared_ptr<services::LogManager> logManager, uint64_t bytes_per_ms) {
    web::json::value log;
    logManager->add_log(enums::LogTypeEnum::EXPERIMENT_INITIALISE, log);

    std::chrono::time_point<std::chrono::system_clock> start_time =
            std::chrono::system_clock::now() + std::chrono::seconds(3);

    auto milliseconds_since_epoch =
            std::chrono::duration_cast<std::chrono::milliseconds>(start_time.time_since_epoch());

    uint64_t millis = milliseconds_since_epoch.count();

    json::value result;
    result["start_time"] = web::json::value::number(millis);
    result["bytes_per_ms"] = web::json::value::number(bytes_per_ms);

    // Use random_device to obtain a seed for the random number engine
    std::random_device rd;

    uint64_t stop_range = int(FRAME_RATE * 1000);

    // Use the seed to initialize the random number engine
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint64_t> distribution(0, stop_range);

    for (const auto &hostName: hosts) {
        try {
            uint64_t variance = distribution(gen);
            variance = 0;

            std::vector<web::json::value> tempHostList;

            for(const auto &otherHost: hosts){
                if(otherHost != hostName)
                    tempHostList.push_back(web::json::value::string(otherHost));
            }

            result["client_list"] = web::json::value::array(tempHostList);

            web::http::http_request request(web::http::methods::POST);
            request.headers().set_content_type(U("application/json"));
            request.set_body(result);
//            result["start_time"] = web::json::value::number(millis + variance);

            auto client = http::client::http_client("http://" + hostName + ":" + std::string(LOW_COMP_PORT) +
                                                    U("/" + std::string(SET_EXPERIMENT_START))).request(
                    request);


            client.wait();
        }
        catch (std::exception e) {
            std::cout << e.what() << std::endl;
        }
        catch (std::logic_error e) {
            std::cout << e.what() << std::endl;
        }
    }
    for (const auto &hostName: hosts) {
        try {
            auto client = http::client::http_client("http://" + hostName + ":" + std::string(HIGH_CLIENT_PORT)).request(
                    http::methods::GET,
                    U("/" + std::string(EXPERIMENT_START)));
            client.wait();
        }
        catch (std::exception e) {
            std::cout << e.what() << std::endl;
        }
        catch (std::logic_error e) {
            std::cout << e.what() << std::endl;
        }
    }
}

MasterController::MasterController(std::shared_ptr<services::LogManager> ptr,
                                   services::WorkQueueManager *sharedPtr) {
    MasterController::logManager = ptr;
    MasterController::workQueueManager = sharedPtr;
}
