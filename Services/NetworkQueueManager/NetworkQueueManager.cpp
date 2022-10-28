//
// Created by jamiec on 10/11/22.
//

#include "NetworkQueueManager.h"
#include "../../utils/MultipartParser/MultipartParser.h"
#include "cpprest/http_client.h"
//#include "date/date.h"
#include <cpprest/filestream.h>

#include <utility>
#include <filesystem>
#include "../../Constants/CLIENT_DETAILS.h"
#include "../../utils/UtilFunctions/UtilFunctions.h"

using namespace std;
using namespace web;
using namespace chrono;

const std::mutex &NetworkQueueManager::getNetworkMutex() const {
    return networkMutex;
}

[[noreturn]] void NetworkQueueManager::initNetworkCommLoop() {
    std::unique_lock<std::mutex> lock(networkMutex, std::defer_lock);
    while (true) {
        lock.lock();

        if (comms.front()->getCommTime() == std::chrono::system_clock::now()) {
            std::shared_ptr<model::BaseNetworkCommsModel> comms_model = comms.front();
            model::BaseNetworkCommsModel tmp_comm = *comms_model;
            comms.erase(comms.begin());
            lock.unlock();
            switch (tmp_comm.getType()) {
                case enums::network_comms_types::halt_req:
                    haltReq(tmp_comm);
                    break;
                case enums::network_comms_types::task_mapping:
                    taskMapping(tmp_comm);
                    break;
                case enums::network_comms_types::task_update:
                    taskUpdate(tmp_comm);
                    break;
            }
        }

        lock.unlock();
    }
}

void NetworkQueueManager::taskUpdate(model::BaseNetworkCommsModel comm_model) {
    std::shared_ptr<model::BaseResult> br = comm_model.getAllocatedTask();
    std::string hostName = comm_model.getHosts()[0];
    std::string current_block = to_string(br->current_block.first) + "_" + to_string(br->current_block.second);

    std::shared_ptr<json::value> output = generateTaskJson(comm_model, current_block);

    auto client = http::client::http_client(hostName).request(http::methods::POST,
                                                              U(":" + std::string(CLIENT_PORT) + "/" + TASK_UPDATE),
                                                              output->serialize());

    client.wait();
}

void NetworkQueueManager::taskMapping(model::BaseNetworkCommsModel comm_model) {
    std::shared_ptr<model::BaseResult> br = comm_model.getAllocatedTask();
    std::string hostName = comm_model.getHosts()[0];

    std::string current_block = "0_0";

    std::shared_ptr<json::value> output = generateTaskJson(comm_model, current_block);

    auto fileStream = std::make_shared<concurrency::streams::ostream>();

    pplx::task<void> requestTask = concurrency::streams::fstream::open_ostream(U(comm_model.getInputFilePath())).then(
                    [=](concurrency::streams::ostream outFile) {
                        *fileStream = outFile;

                        //Use MultipartParser to get the encoded body content and boundary
                        web::http::MultipartParser parser;
                        parser.AddParameter("Filename",
                                            std::filesystem::path(comm_model.getInputFilePath()).filename());
                        parser.AddFile("file", comm_model.getInputFilePath());
                        std::string boundary = parser.boundary();
                        std::string body = parser.GenBodyContent();
                        std::cout << body << std::endl;

                        //Set up http client and request
                        web::http::http_request req;
                        web::http::client::http_client client(U(hostName + ":" + CLIENT_PORT + "/" + TASK_ALLOCATION));
                        req.set_method(web::http::methods::POST);
                        req.set_body(body, "multipart/form-data; boundary=" + boundary);
                        return client.request(req);
                    }).then([=](const pplx::task<web::http::http_response> &response_task) {
                web::http::http_response response = response_task.get();
                return response.body().read_to_end(fileStream->streambuf());
            })
            .then([=](size_t) {
                return fileStream->close();
            });

    // Wait for all the outstanding I/O to complete and handle any exceptions
    try {
        requestTask.wait();
    }
    catch (const std::exception &e) {
        printf("Error exception:%s\n", e.what());
    }

}

void NetworkQueueManager::haltReq(model::BaseNetworkCommsModel comm_model) {
    std::vector<std::string> hosts = comm_model.getHosts();

    for (const auto &host: hosts) {

        auto client = web::http::client::http_client(host).request(web::http::methods::POST,
                                                                   ":" + std::string(CLIENT_PORT) + "/" +
                                                                   std::string(HALT_ENDPOINT))
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
        comms.clear();
        comms.push_back(comm_model);
    }


    lock.unlock();
}

std::shared_ptr<web::json::value>
NetworkQueueManager::generateTaskJson(model::BaseNetworkCommsModel comm_model, std::string current_block) {
    std::shared_ptr<model::BaseResult> br = comm_model.getAllocatedTask();

    std::shared_ptr<json::value> output;
    (*output)["type"] = json::value(static_cast<bool>(br->getDnnType()));
    (*output)["source"] = json::value(br->getSrcHost());
    (*output)["deadline"] = json::value(
            utils::convertDateToString((br->getDeadline())));
    (*output)["estimated_start_time"] = json::value(
            utils::convertDateToString(br->getEstimatedStart()));
    (*output)["estimated_start_time"] = json::value(
            utils::convertDateToString(br->getEstimatedStart()));
    (*output)["estimated_finish_time"] = json::value(
            utils::convertDateToString(br->getEstimatedFinish()));
    (*output)["dnn_id"] = json::value(br->getDnnId());

    (*output)["current_block"] = json::value(std::move(current_block));

    auto group_blocks_val = json::value();

    for (auto &[outer_key, outer_value]: br->tasks) {
        auto group_block = json::value();
        for (auto &[inner_key, inner_value]: outer_value) {
            auto block = json::value();
            block["estimated_finish_time"] = json::value(utils::convertDateToString(inner_value->getEstimatedFinish()));
            block["estimated_start_time"] = json::value(utils::convertDateToString(inner_value->getEstimatedFinish()));
            block["group_block_id"] = json::value(inner_value->getGroupBlockId());
            block["block_id"] = json::value(inner_value->getPartitionModelId());
            block["allocated_device"] = json::value(inner_value->getAllocatedHost());

            auto inner_layer_ids = vector<json::value>();

            for (auto num: inner_value->getOriginalLayerIds())
                inner_layer_ids.emplace_back(num);

            block["original_layer_ids"] = json::value::array(inner_layer_ids);

            auto input_tile_region = json::value();
            input_tile_region["x1"] = inner_value->getInMap()->getX1();
            input_tile_region["x2"] = inner_value->getInMap()->getX2();
            input_tile_region["y1"] = inner_value->getInMap()->getY1();
            input_tile_region["y2"] = inner_value->getInMap()->getY2();
            block["in_map"] = input_tile_region;

            group_block[inner_key] = block;
        }
        group_blocks_val[outer_key] = group_block;
    }
    return output;
}


