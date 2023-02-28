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
#include <thread>
#include "../Constants/CLIENT_DETAILS.h"
#include "../utils/IPerfTest/IPerfTest.h"
#include "../model/data_models/WorkItems/ProcessingItem/LowProcessingItem/LowProcessingItem.h"


using namespace web;
using namespace http;

void MasterController::handle_get(http_request message) {
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

void MasterController::handle_post(http_request message) {
    auto path = std::string(message.relative_uri().path());
    auto body = message.extract_json().get();
    std::cout << "INBOUND REQ: " << path << std::endl;
    try {
        if (!path.empty()) {
            std::cout << "REQUEST_RECEIVED" << std::endl;
            if (path == LOG_RESULT) {
                std::string result = MasterController::logManager->write_log();
                message.reply(status_codes::OK, result).wait();
            } else if (path == HIGH_OFFLOAD_REQUEST) {
                message.reply(status_codes::OK);
                enums::request_type requestType = enums::request_type::high_complexity;
                uint64_t deadline_ms = body["deadline"].as_number().to_uint64();
                std::chrono::time_point<std::chrono::system_clock> deadline{
                        std::chrono::milliseconds(deadline_ms)};

                std::shared_ptr<std::vector<std::string>> hostList = std::make_shared<std::vector<std::string>>(
                        std::initializer_list<std::string>{message.remote_address()});

                std::string dnn_id = body["dnn_id"].as_string();
                dnn_id = message.remote_address() + "_" + dnn_id;

                std::shared_ptr<model::HighProcessingItem> highProcessingItem = std::make_shared<model::HighProcessingItem>(
                        hostList,
                        requestType,
                        deadline,
                        dnn_id);

                workQueueManager->add_task(std::static_pointer_cast<model::WorkItem>(highProcessingItem));
                web::json::value log;
                log["dnn_id"] = web::json::value::string(dnn_id);
                log["source_host"] = web::json::value::string(message.remote_address());
                log["deadline"] = web::json::value::number(body["deadline"].as_number().to_uint64());
                MasterController::logManager->add_log(enums::LogTypeEnum::HIGH_COMP_REQUEST, log);
                return;

            } else if (path == LOW_OFFLOAD_REQUEST) {
                enums::request_type requestType = enums::request_type::low_complexity;

                std::chrono::time_point<std::chrono::system_clock> deadline{
                        std::chrono::milliseconds(body["deadline"].as_number().to_uint64())};

                std::string dnn_id = body["dnn_id"].as_string();

                std::string sourceDevice = message.remote_address();
                dnn_id = sourceDevice + "_" + dnn_id;

                std::shared_ptr<std::vector<std::string>> hostList = std::make_shared<std::vector<std::string>>(
                        std::initializer_list<std::string>{sourceDevice});

                std::shared_ptr<model::LowProcessingItem> lowProcessingItem = std::make_shared<model::LowProcessingItem>(
                        hostList, requestType, deadline,
                        std::make_pair(dnn_id, sourceDevice));

                web::json::value log;
                log["dnn_id"] = web::json::value::string(dnn_id);
                log["source_host"] = web::json::value::string(message.remote_address());
                log["deadline"] = web::json::value::number(body["deadline"].as_number().to_uint64());
                MasterController::logManager->add_log(enums::LogTypeEnum::LOW_COMP_REQUEST, log);
                MasterController::workQueueManager->add_task(lowProcessingItem);
                http_response response;
                response.set_status_code(status_codes::Created);
                message.reply(response);
            } else if (path == DEVICE_REGISTER_REQUEST) {
                dev_list->register_device(message.remote_address());
                std::shared_ptr<model::ComputationDevice> computationDevice = std::make_shared<model::ComputationDevice>(
                        PER_DEVICE_CORES, message.remote_address());
                workQueueManager->network->devices[message.remote_address()] = computationDevice;
                workQueueManager->networkQueueManager->hosts.push_back(message.remote_address());
                web::json::value response;
                response["host_name"] = json::value::string(message.remote_address());
                message.reply(status_codes::Created, response);

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
                    workQueueManager->networkQueueManager->addTask(baseNetworkCommsModel);
                }

            } else if (path == DAG_DISRUPTION_PATH) {
                message.reply(status_codes::OK);
                std::string serialised_test = body.serialize();
                std::cout << serialised_test << std::endl;
                enums::request_type requestType = enums::request_type::dag_disruption;
                std::string host = message.remote_address();

                std::string dnn_id = body["dnn_id"].as_string();
                std::string convidx = body["convidx"].as_string();
                int partition_id = body["partition_id"].as_integer();

                std::shared_ptr<std::vector<std::string>> hostList = std::make_shared<std::vector<std::string>>(
                        std::initializer_list<std::string>{host});


                std::chrono::time_point<std::chrono::system_clock> finish_time{
                        std::chrono::milliseconds(body["finish_time"].as_number().to_uint64())};

                web::json::value log;
                log["dnn_id"] = web::json::value::string(dnn_id);
                log["finish_time"] = web::json::value::number(body["finish_time"].as_number().to_uint64());
                log["convidx"] = web::json::value::string(convidx);
                log["partition_id"] = web::json::value::number(partition_id);
                MasterController::logManager->add_log(enums::LogTypeEnum::DAG_DISRUPTION_REQUEST, log);


                std::shared_ptr<model::DAGDisruption> dagDisruption = std::make_shared<model::DAGDisruption>(hostList,
                                                                                                             requestType,
                                                                                                             dnn_id,
                                                                                                             convidx,
                                                                                                             partition_id,
                                                                                                             finish_time);

                workQueueManager->add_task(std::static_pointer_cast<model::WorkItem>(dagDisruption));
            } else if (path == STATE_UPDATE) {
                message.reply(status_codes::OK);
                std::string body_dump = body.serialize();
                std::cout << body_dump <<std::endl;
                std::string host = message.remote_address();

                std::shared_ptr<std::vector<std::string>> hostList = std::make_shared<std::vector<std::string>>(
                        std::initializer_list<std::string>{host});

                enums::request_type requestType = enums::request_type::state_update;


                std::map<int, std::map<std::string, std::chrono::time_point<std::chrono::system_clock>>> finish_map;

                auto finish_list = body["finish_times"].as_array();
                std::string convidx = body["convidx"].as_string();
                std::string dnn_id = body["dnn_id"].as_string();

                web::json::value log;
                log["dnn_id"] = web::json::value::string(dnn_id);
                log["convidx"] = web::json::value::string(convidx);
                log["finish_times"] = web::json::value();

                for (auto finish_item: finish_list) {
                    std::map<std::string, std::chrono::time_point<std::chrono::system_clock>> detail_map;
                    auto finish_object = finish_item.as_object();
                    int partition_id = finish_object["partition_id"].as_integer();
                    uint64_t finish_time = finish_object["finish_time"].as_number().to_uint64();
                    uint64_t assembly_upload_start = finish_object["assembly_upload_start"].as_number().to_uint64();
                    uint64_t assembly_upload_finish = finish_object["assembly_upload_finish"].as_number().to_uint64();
                    uint64_t task_forward_start = finish_object["task_forward_start"].as_number().to_uint64();
                    uint64_t task_forward_finish = finish_object["task_forward_finish"].as_number().to_uint64();
                    detail_map["finish_time"] = std::chrono::time_point<std::chrono::system_clock>(
                            std::chrono::milliseconds{finish_time});
                    detail_map["assembly_upload_start"] = std::chrono::time_point<std::chrono::system_clock>(
                            std::chrono::milliseconds{assembly_upload_start});
                    detail_map["assembly_upload_finish"] = std::chrono::time_point<std::chrono::system_clock>(
                            std::chrono::milliseconds{assembly_upload_finish});
                    detail_map["task_forward_start"] = std::chrono::time_point<std::chrono::system_clock>(
                            std::chrono::milliseconds{task_forward_start});
                    detail_map["task_forward_finish"] = std::chrono::time_point<std::chrono::system_clock>(
                            std::chrono::milliseconds{task_forward_finish});
                    finish_map[partition_id] = detail_map;


                    log["finish_times"][std::to_string(partition_id)] = web::json::value();
                    log["finish_times"][std::to_string(partition_id)]["finish_time"] = web::json::value::number(finish_time);
                    log["finish_times"][std::to_string(partition_id)]["assembly_upload_start"] = web::json::value::number(assembly_upload_start);
                    log["finish_times"][std::to_string(partition_id)]["assembly_upload_finish"] = web::json::value::number(assembly_upload_finish);
                    log["finish_times"][std::to_string(partition_id)]["task_forward_start"] = web::json::value::number(task_forward_start);
                    log["finish_times"][std::to_string(partition_id)]["task_forward_finish"] = web::json::value::number(task_forward_finish);
                }

                MasterController::logManager->add_log(enums::LogTypeEnum::STATE_UPDATE_REQUEST, log);

                auto stateUpdateItem = std::make_shared<model::StateUpdate>(hostList, requestType, finish_map, convidx,
                                                                            dnn_id);

                workQueueManager->add_task(std::static_pointer_cast<model::WorkItem>(stateUpdateItem));
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
