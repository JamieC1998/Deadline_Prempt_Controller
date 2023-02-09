//
// Created by jamiec on 10/11/22.
//

#include "NetworkQueueManager.h"
#include "../../utils/MultipartParser/MultipartParser.h"
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

using namespace std;
using namespace web;
using namespace chrono;

namespace services {
    std::mutex &NetworkQueueManager::getNetworkMutex() {
        return networkMutex;
    }

    [[noreturn]] void NetworkQueueManager::initNetworkCommLoop() {
        std::unique_lock<std::mutex> lock(networkMutex, std::defer_lock);
        while (true) {
            lock.lock();

            if (comms.front()->getCommTime() <= std::chrono::system_clock::now()) {
                std::shared_ptr<model::BaseNetworkCommsModel> comms_model = comms.front();
                comms.erase(comms.begin());
                lock.unlock();
                switch (comms_model->getType()) {
                    case enums::network_comms_types::halt_req:
                        haltReq(comms_model, this);
                        break;
                    case enums::network_comms_types::high_complexity_task_mapping:
                        highTaskAllocation(comms_model, this);
                        break;
                    case enums::network_comms_types::task_update:
                        taskUpdate(comms_model, this);
                        break;
                    case enums::network_comms_types::low_complexity_allocation:
                        lowTaskAllocation(comms_model, this);
                        break;
                    case enums::network_comms_types::high_complexity_task_reallocation:
                        highTaskReallocation(comms_model, this);
                        break;
                }
            } else
                lock.unlock();
        }
    }

    void taskUpdate(std::shared_ptr<model::BaseNetworkCommsModel> comm_model, NetworkQueueManager *queueManager) {
        auto taskUpdate = std::static_pointer_cast<model::OutboundUpdate>(comm_model);
        std::shared_ptr<model::HighCompResult> br = taskUpdate->getDnn();

        std::string conv_block_to_update = taskUpdate->getUpdateConvidx();

        json::value taskStructure = br->convertToJson();
        json::value result;
        result[U("conv_idx")] = json::value::string(conv_block_to_update);
        result[U("dnn")] = json::value(taskStructure);
        result[U("timestamp")] = json::value::number(std::chrono::system_clock::now().time_since_epoch().count() * 1000);

        for (auto hostName: queueManager->getHosts()) {
            result[U("host")] = json::value::string(hostName);
            auto client = http::client::http_client(hostName).request(http::methods::POST,
                                                                      U(":" + std::string(HIGH_CLIENT_PORT) + "/" +
                                                                        TASK_UPDATE),
                                                                      result.serialize()).then(
                    [](web::http::http_response response) {
                        // No need to wait, just log the status code
                        std::cout << "Status code: " << response.status_code() << std::endl;
                    });

            client.wait();
        }
    }

    void
    highTaskAllocation(std::shared_ptr<model::BaseNetworkCommsModel> comm_model, NetworkQueueManager *queueManager) {
        auto highCompComm = std::static_pointer_cast<model::HighComplexityAllocationComms>(comm_model);
        std::shared_ptr<model::HighCompResult> br = highCompComm->getAllocatedTask();
        std::string hostName = highCompComm->getHost();

        std::string current_conv = br->getStartingConvidx();

        json::value task_json = br->convertToJson();

        json::value output;
        output[U("starting_conv")] = json::value::string(current_conv);
        output[U("dnn")] = json::value(task_json);

        http::client::http_client(hostName).request(http::methods::POST,
                                                    U(":" + std::string(HIGH_CLIENT_PORT) + "/" +
                                                      HIGH_TASK_ALLOCATION),
                                                    output.serialize()).then(
                [](web::http::http_response response) {
                    // No need to wait, just log the status code
                    std::cout << "Status code: " << response.status_code() << std::endl;
                });
    }

    void haltReq(std::shared_ptr<model::BaseNetworkCommsModel> comm_model, NetworkQueueManager *queueManager) {
        auto haltCommModel = static_pointer_cast<model::HaltNetworkCommsModel>(comm_model);

        web::json::value result;

        for(auto [key, value]: haltCommModel->getVersionMap())
            result[key] = web::json::value::string(value);

        for (const auto &host: queueManager->getHosts()) {

            auto client = web::http::client::http_client(host).request(web::http::methods::POST,
                                                                       ":" + std::string(HIGH_CLIENT_PORT) + "/" +
                                                                       std::string(HALT_ENDPOINT), result.serialize())
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
    lowTaskAllocation(std::shared_ptr<model::BaseNetworkCommsModel> comm_model, NetworkQueueManager *queueManager) {
        auto lowCompComm = static_pointer_cast<model::LowComplexityAllocationComms>(comm_model);
        auto task_res = lowCompComm->getAllocatedTask();
        std::string host = lowCompComm->getHost();

        json::value result;
        result["dnn_id"] = json::value::string(task_res->getDnnId());
        result["start_time"] = json::value::number(std::chrono::duration_cast<std::chrono::milliseconds>(
                task_res->getEstimatedStart().time_since_epoch()).count());
        result["finish_time"] = json::value::number(std::chrono::duration_cast<std::chrono::milliseconds>(
                task_res->getEstimatedFinish().time_since_epoch()).count());

        web::http::client::http_client(host).request(web::http::methods::POST,
                                                     ":" + std::string(LOW_COMP_PORT) + "/" +
                                                     std::string(LOW_TASK_ALLOCATION), result.serialize())
                .then([](web::http::http_response response) {
                    std::cout << "Status code: " << response.status_code() << std::endl;
                });
    }

    void
    highTaskReallocation(std::shared_ptr<model::BaseNetworkCommsModel> comm_model, NetworkQueueManager *queueManager) {
        auto reallocationComm = static_pointer_cast<model::HighComplexityAllocationComms>(comm_model);
        auto hostName = reallocationComm->getHost();
        auto dnn = reallocationComm->getAllocatedTask();
        auto conv_idx_to_update = std::to_string(dnn->getLastCompleteConvIdx() + 1);

        auto task_json = dnn->convertToJson();
        json::value output;
        output[U("starting_conv")] = json::value::string(conv_idx_to_update);
        output[U("dnn")] = json::value(task_json);

        http::client::http_client(hostName).request(http::methods::POST,
                                                    U(":" + std::string(HIGH_CLIENT_PORT) + "/" +
                                                      HIGH_TASK_REALLOCATION),
                                                    output.serialize()).then(
                [](web::http::http_response response) {
                    // No need to wait, just log the status code
                    std::cout << "Status code: " << response.status_code() << std::endl;
                });
    }


    void NetworkQueueManager::addTask(std::shared_ptr<model::BaseNetworkCommsModel> comm_model) {
        std::unique_lock<std::mutex> lock(networkMutex, std::defer_lock);
        lock.lock();

        if (comm_model->getType() != enums::network_comms_types::halt_req) {
            comms.push_back(comm_model);
            std::sort(comms.begin(), comms.end(), [](const std::shared_ptr<model::BaseNetworkCommsModel> &a,
                                                     const std::shared_ptr<model::BaseNetworkCommsModel> &b) -> bool {
                return a->getCommTime() < b->getCommTime();
            });
        } else {
            //TODO PRUNE EVERYTHING EXCEPT LOW COMP
            comms.clear();
            comms.push_back(comm_model);
        }

        lock.unlock();
    }

    const vector <std::string> &NetworkQueueManager::getHosts() const {
        return hosts;
    }

    NetworkQueueManager::NetworkQueueManager(const vector <std::string> &hosts) : hosts(hosts) {}
}
