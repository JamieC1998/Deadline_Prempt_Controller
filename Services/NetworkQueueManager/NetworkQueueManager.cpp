//
// Created by jamiec on 10/11/22.
//

#include "NetworkQueueManager.h"
#include "cpprest/http_client.h"
#include <cpprest/filestream.h>

#include <utility>
#include <filesystem>
#include "../../Constants/CLIENT_DETAILS.h"
#include "../../utils/UtilFunctions/UtilFunctions.h"
#include "../../model/data_models/NetworkCommsModels/HighComplexityAllocation/HighComplexityAllocationComms.h"
#include "../../model/data_models/NetworkCommsModels/OutboundUpdate/OutboundUpdate.h"
#include "../../model/data_models/NetworkCommsModels/LowComplexityAllocation/LowComplexityAllocationComms.h"
#include "../../model/data_models/NetworkCommsModels/HaltNetworkCommsModel/HaltNetworkCommsModel.h"
#include "../../model/data_models/NetworkCommsModels/OutboundPrune/OutboundPrune.h"

using namespace std;
using namespace web;
using namespace chrono;

namespace services {
    std::mutex &NetworkQueueManager::getNetworkMutex() {
        return networkMutex;
    }

    [[noreturn]] void
    NetworkQueueManager::initNetworkCommLoop(std::shared_ptr<services::NetworkQueueManager> queueManager) {
        try {
            std::unique_lock<std::mutex> lock(queueManager->networkMutex, std::defer_lock);
            while (true) {
                lock.lock();

                if (!queueManager->comms.empty() &&
                    queueManager->comms.front()->getCommTime() <= std::chrono::system_clock::now()) {
                    std::shared_ptr<model::BaseNetworkCommsModel> comms_model = queueManager->comms.front();
                    queueManager->comms.erase(queueManager->comms.begin());
                    lock.unlock();
                    switch (comms_model->getType()) {
                        case enums::network_comms_types::halt_req:
                            haltReq(comms_model, queueManager);
                            break;
                        case enums::network_comms_types::high_complexity_task_mapping:
                            highTaskAllocation(comms_model, queueManager);
                            break;
                        case enums::network_comms_types::task_update:
                            taskUpdate(comms_model, queueManager);
                            break;
                        case enums::network_comms_types::low_complexity_allocation:
                            lowTaskAllocation(comms_model, queueManager);
                            break;
                        case enums::network_comms_types::high_complexity_task_reallocation:
                            highTaskReallocation(comms_model, queueManager);
                            break;
                        case enums::network_comms_types::initial_experiment_start:
                            initialise_experiment(queueManager);
                            break;
                        case enums::network_comms_types::prune_dnn:
                            pruneDNN(comms_model, queueManager);
                            break;
                    }
                } else
                    lock.unlock();
            }
        }
        catch (std::exception &e) {
            std::cerr << "something wrong has happened! ;)" << '\n';
            std::cerr << e.what() << "\n";
        }
    }

    void initialise_experiment(std::shared_ptr<services::NetworkQueueManager> queueManager) {
        web::json::value log;
        queueManager->logManager->add_log(enums::LogTypeEnum::EXPERIMENT_INITIALISE, log);

        std::chrono::time_point<std::chrono::system_clock> start_time =
                std::chrono::system_clock::now() + std::chrono::seconds(3);

        auto milliseconds_since_epoch =
                std::chrono::duration_cast<std::chrono::milliseconds>(start_time.time_since_epoch());

        uint64_t millis = milliseconds_since_epoch.count();

        json::value result;
        result["start_time"] = web::json::value::number(millis);

        for (const auto &hostName: queueManager->getHosts()) {
            auto client = http::client::http_client("http://" + hostName + ":" + std::string(LOW_COMP_PORT)).request(
                    http::methods::POST,
                    U("/" + std::string(SET_EXPERIMENT_START)),
                    result.serialize()).then(
                    [](web::http::http_response response) {
                        // No need to wait, just log the status code
                        std::cout << "Status code: " << response.status_code() << std::endl;
                    });

            client.wait();
        }
    }

    void taskUpdate(std::shared_ptr<model::BaseNetworkCommsModel> comm_model,
                    std::shared_ptr<services::NetworkQueueManager> queueManager) {
        auto taskUpdate = std::static_pointer_cast<model::OutboundUpdate>(comm_model);
        std::shared_ptr<model::HighCompResult> br = taskUpdate->getDnn();

        std::string conv_block_to_update = taskUpdate->getUpdateConvidx();

        json::value taskStructure = br->convertToJson();
        json::value result;
        result[U("dnn")] = json::value(taskStructure);
        result[U("old_version")] = json::value::number(taskUpdate->getOldVersion());

        web::json::value log;
        log["old_version_id"] = web::json::value::number(taskUpdate->getOldVersion());
        log["dnn"] = web::json::value(taskStructure);
        log["comm_time"] = web::json::value::number(std::chrono::duration_cast<std::chrono::milliseconds>(
                comm_model->getCommTime().time_since_epoch()).count() * 1000);
        queueManager->logManager->add_log(enums::LogTypeEnum::OUTBOUND_STATE_UPDATE, log);

        for (const auto &hostName: queueManager->getHosts()) {
            result[U("host")] = json::value::string(hostName);
            auto client = http::client::http_client("http://" + hostName + ":" + std::string(HIGH_CLIENT_PORT)).request(
                    http::methods::POST, "/" +
                                         std::string(TASK_UPDATE),
                    result.serialize()).then(
                    [](web::http::http_response response) {
                        // No need to wait, just log the status code
                        std::cout << "Status code: " << response.status_code() << std::endl;
                    });

            client.wait();
        }
    }

    void
    highTaskAllocation(std::shared_ptr<model::BaseNetworkCommsModel> comm_model,
                       std::shared_ptr<services::NetworkQueueManager> queueManager) {
        auto highCompComm = std::static_pointer_cast<model::HighComplexityAllocationComms>(comm_model);
        std::shared_ptr<model::HighCompResult> br = highCompComm->getAllocatedTask();
        std::string hostName = highCompComm->getHost();

        std::string current_conv = br->getStartingConvidx();

        json::value task_json = br->convertToJson();

        json::value output;

        web::json::value log;
        log["dnn"] = web::json::value(br->convertToJson());
        log["comm_time"] = web::json::value::number(std::chrono::duration_cast<std::chrono::milliseconds>(
                comm_model->getCommTime().time_since_epoch()).count() * 1000);
        queueManager->logManager->add_log(enums::LogTypeEnum::OUTBOUND_TASK_ALLOCATION_HIGH, log);

        output[U("starting_conv")] = json::value::string(current_conv);
        output[U("dnn")] = json::value(task_json);

        std::chrono::time_point<std::chrono::system_clock> comm_start = std::chrono::system_clock::now();
        std::string baseURI = "http://" + hostName + ":" + std::string(HIGH_CLIENT_PORT);
        std::cout << "HIGH_COMP_ALLOCATION: " << baseURI << "/" << std::string(HIGH_TASK_ALLOCATION) << std::endl;
        http::client::http_client(baseURI).request(
                http::methods::POST,
                "/" +
                std::string(HIGH_TASK_ALLOCATION),
                output.serialize()).then(
                [comm_start, br, highCompComm](web::http::http_response response) {
                    try {
                        std::mutex* mut = highCompComm->getMut();
                        std::unique_lock uniqueLock(*mut, std::defer_lock);
                        uniqueLock.lock();
                        br->getUploadData()->setActualStartFinTime(std::make_pair(comm_start, std::chrono::system_clock::now()));
                        uniqueLock.unlock();
                        std::cout << "Status code: " << response.status_code() << std::endl;
                    }
                    catch (exception e) {
                        std::cout << e.what() << std::endl;
                    }
                });

    }

    void
    highTaskReallocation(std::shared_ptr<model::BaseNetworkCommsModel> comm_model,
                         std::shared_ptr<services::NetworkQueueManager> queueManager) {
        auto reallocationComm = static_pointer_cast<model::HighComplexityAllocationComms>(comm_model);
        auto hostName = reallocationComm->getHost();
        auto dnn = reallocationComm->getAllocatedTask();
        auto conv_idx_to_update = std::to_string(dnn->getLastCompleteConvIdx() + 1);

        auto task_json = dnn->convertToJson();
        json::value output;
        output[U("starting_conv")] = json::value::string(conv_idx_to_update);
        output[U("dnn")] = json::value(task_json);

        web::json::value log;
        log["dnn"] = web::json::value(dnn->convertToJson());
        log["comm_time"] = web::json::value::number(std::chrono::duration_cast<std::chrono::milliseconds>(
                comm_model->getCommTime().time_since_epoch()).count() * 1000);
        queueManager->logManager->add_log(enums::LogTypeEnum::OUTBOUND_TASK_REALLOCATION_HIGH, log);


        http::client::http_client("http://" + hostName + ":" + std::string(HIGH_CLIENT_PORT)).request(
                http::methods::POST,
                "/" +
                std::string(HIGH_TASK_REALLOCATION),
                output.serialize()).then(
                [](web::http::http_response response) {
                    // No need to wait, just log the status code
                    std::cout << "Status code: " << response.status_code() << std::endl;
                });
    }


    void pruneDNN(std::shared_ptr<model::BaseNetworkCommsModel> comm_model,
                  std::shared_ptr<services::NetworkQueueManager> queueManager) {
        auto pruneModel = static_pointer_cast<model::OutboundPrune>(comm_model);
        std::string dnn_id = pruneModel->getDnnId();

        web::json::value result;
        web::json::value log;
        log["dnn_id"] = web::json::value::string(dnn_id);
        log["comm_time"] = web::json::value::number(std::chrono::duration_cast<std::chrono::milliseconds>(
                comm_model->getCommTime().time_since_epoch()).count() * 1000);
        result["dnn_id"] = web::json::value::string(dnn_id);

        queueManager->logManager->add_log(enums::LogTypeEnum::OUTBOUND_PRUNE, log);

        for (const auto &host: queueManager->getHosts()) {
            auto client = web::http::client::http_client(
                    "http://" + host + ":" + std::string(HIGH_CLIENT_PORT)).request(web::http::methods::POST,
                                                                                    "/" +
                                                                                    std::string(PRUNE_ENDPOINT),
                                                                                    result.serialize())
                    .then([](web::http::http_response response) {
                    });
        }

    }


    void haltReq(std::shared_ptr<model::BaseNetworkCommsModel> comm_model,
                 std::shared_ptr<services::NetworkQueueManager> queueManager) {
        auto haltCommModel = static_pointer_cast<model::HaltNetworkCommsModel>(comm_model);

        web::json::value result;

        web::json::value log;
        log["comm_time"] = web::json::value::number(std::chrono::duration_cast<std::chrono::milliseconds>(
                comm_model->getCommTime().time_since_epoch()).count() * 1000);
        queueManager->logManager->add_log(enums::LogTypeEnum::OUTBOUND_HALT_REQUEST, log);

        for (auto [key, value]: haltCommModel->getVersionMap())
            result[key] = web::json::value::number(value);

        for (const auto &host: queueManager->getHosts()) {

            auto client = web::http::client::http_client(
                    "http://" + host + ":" + std::string(HIGH_CLIENT_PORT)).request(web::http::methods::POST,
                                                                                    "/" +
                                                                                    std::string(HALT_ENDPOINT),
                                                                                    result.serialize())
                    .then([](web::http::http_response response) {
                    });

            // Wait for the concurrent tasks to finish.
            try {
                client.wait();
            }
            catch (const std::exception &e) {
                printf("Error exception:%s\n", e.what());
            }
        }
    }

    void
    lowTaskAllocation(std::shared_ptr<model::BaseNetworkCommsModel> comm_model,
                      std::shared_ptr<services::NetworkQueueManager> queueManager) {
        auto lowCompComm = static_pointer_cast<model::LowComplexityAllocationComms>(comm_model);
        auto task_res = lowCompComm->getAllocatedTask();
        std::string host = lowCompComm->getHost();

        json::value result;
        result["dnn_id"] = json::value::string(task_res->getDnnId());
        result["start_time"] = json::value::number(std::chrono::duration_cast<std::chrono::milliseconds>(
                task_res->getEstimatedStart().time_since_epoch()).count());
        result["finish_time"] = json::value::number(std::chrono::duration_cast<std::chrono::milliseconds>(
                task_res->getEstimatedFinish().time_since_epoch()).count());

        web::json::value log;
        log["dnn"] = web::json::value(task_res->convertToJson());
        log["comm_time"] = web::json::value::number(std::chrono::duration_cast<std::chrono::milliseconds>(
                comm_model->getCommTime().time_since_epoch()).count() * 1000);
        queueManager->logManager->add_log(enums::LogTypeEnum::OUTBOUND_LOW_COMP_ALLOCATION, log);

        web::http::client::http_client("http://" + host + ":" + std::string(LOW_COMP_PORT)).request(
                        web::http::methods::POST,
                        "/" + std::string(LOW_TASK_ALLOCATION), result.serialize())
                .then([](web::http::http_response response) {
                    std::cout << "Status code: " << response.status_code() << std::endl;
                });
    }

    void NetworkQueueManager::addTask(std::shared_ptr<model::BaseNetworkCommsModel> comm_model) {
        std::unique_lock<std::mutex> lock(NetworkQueueManager::networkMutex, std::defer_lock);
        lock.lock();

        web::json::value log;
        NetworkQueueManager::logManager->add_log(enums::LogTypeEnum::ADD_NETWORK_TASK, log);

        if (comm_model->getType() != enums::network_comms_types::halt_req) {
            NetworkQueueManager::comms.push_back(comm_model);
            std::sort(NetworkQueueManager::comms.begin(), NetworkQueueManager::comms.end(),
                      [](const std::shared_ptr<model::BaseNetworkCommsModel> &a,
                         const std::shared_ptr<model::BaseNetworkCommsModel> &b) -> bool {
                          return a->getCommTime() < b->getCommTime();
                      });
        } else {
            std::vector<std::shared_ptr<model::BaseNetworkCommsModel>> new_list;

            for (auto element: NetworkQueueManager::comms)
                if (element->getType() == enums::network_comms_types::low_complexity_allocation)
                    new_list.push_back(element);
            NetworkQueueManager::comms = new_list;
            NetworkQueueManager::comms.push_back(comm_model);

            std::sort(NetworkQueueManager::comms.begin(), NetworkQueueManager::comms.end(),
                      [](const std::shared_ptr<model::BaseNetworkCommsModel> &a,
                         const std::shared_ptr<model::BaseNetworkCommsModel> &b) -> bool {
                          return a->getCommTime() < b->getCommTime();
                      });
        }

        lock.unlock();
    }

    const vector <std::string> &NetworkQueueManager::getHosts() const {
        return hosts;
    }

    NetworkQueueManager::NetworkQueueManager(std::shared_ptr<services::LogManager> ptr) : logManager(ptr) {}
}
