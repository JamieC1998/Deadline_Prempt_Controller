//
// Created by jamiec on 9/27/22.
//

#include "ComputationDevice.h"
#include "../CompResult/HighCompResult/HighCompResult.h"
#include "../CompResult/LowCompResult/LowCompResult.h"
#include "../../../Constants/AllocationMacros.h"
#include "../../../Constants/FTP_CONFIG.h"
#include "../../../utils/UtilFunctions/UtilFunctions.h"

#include <utility>

namespace model {
    int ComputationDevice::id_counter = 0;

    int ComputationDevice::getCores() const {
        return cores;
    }

    void ComputationDevice::setCores(int cores) {
        ComputationDevice::cores = cores;
    }

    const std::string &ComputationDevice::getHostName() const {
        return host_name;
    }

    void ComputationDevice::setHostName(const std::string &hostName) {
        host_name = hostName;
    }

    const std::vector<std::shared_ptr<BaseCompResult>> &ComputationDevice::getDNNs() const {
        return DNNS;
    }

    void ComputationDevice::setTasks(const std::vector<std::shared_ptr<BaseCompResult>> &tasks) {
        DNNS = tasks;
    }

    ComputationDevice::ComputationDevice(int cores, std::string hostName) : cores(
            cores), host_name(hostName), id(id_counter) {
        id_counter++;

        generateDefaultResourceConfig(cores, hostName);

    }

    void ComputationDevice::generateDefaultResourceConfig(int cores, std::string hostName){
        std::chrono::time_point<std::chrono::system_clock> start_time = std::chrono::system_clock::now();
        auto low_comp_res = std::make_shared<model::ResourceAvailabilityList>(
                LOW_COMPLEXITY_CORE_COUNT, cores / LOW_COMPLEXITY_CORE_COUNT, cores,
                std::chrono::milliseconds{LOW_COMPLEXITY_PROCESSING_TIME}, start_time, hostName);

        ComputationDevice::resource_avail_windows[LOW_COMPLEXITY_CORE_COUNT] = low_comp_res;

        auto low_part_res = std::make_shared<model::ResourceAvailabilityList>(
                FTP_LOW_CORE,
                cores /
                FTP_LOW_CORE,
                cores,
                std::chrono::milliseconds{
                        FTP_LOW_TIME},
                start_time,
                hostName);

        ComputationDevice::resource_avail_windows[FTP_LOW_CORE] = low_part_res;

        auto high_part_res = std::make_shared<model::ResourceAvailabilityList>(
                FTP_HIGH_CORE,
                cores /
                FTP_HIGH_CORE,
                cores,
                std::chrono::milliseconds{
                        FTP_HIGH_TIME},
                start_time,
                hostName);

        ComputationDevice::resource_avail_windows[FTP_HIGH_CORE] = high_part_res;
    };

    int ComputationDevice::getId() const {
        return id;
    }

    web::json::value ComputationDevice::convertToJson() {
        web::json::value result;
        result["host_name"] = web::json::value::string(ComputationDevice::getHostName());

        std::vector<web::json::value> dnn_list;
        for (const auto &dnn_task: ComputationDevice::DNNS) {
            if (dnn_task->getDnnType() == enums::dnn_type::high_comp)
                dnn_list.push_back(std::static_pointer_cast<HighCompResult>(dnn_task)->convertToJson());
            else
                dnn_list.push_back(std::static_pointer_cast<LowCompResult>(dnn_task)->convertToJson());
        }
        result["tasks"] = web::json::value::array(dnn_list);
        return result;
    }

    void ComputationDevice::resAvailRemoveAndSplit(std::shared_ptr<model::TimeWindow> tw, int coreUsage, int taskCounter) {

        std::cout << "TASK WINDOW REQ: " << tw->toString() << std::endl;
        for (const auto& avail_pair: ComputationDevice::resource_avail_windows) {
            auto cores = avail_pair.first;
            auto resAvailConfig = avail_pair.second;

            int remaining_coreUsage = coreUsage;
            std::vector<std::shared_ptr<model::ResourceWindow>> res_windows;

            std::cout << "\nBEFORE: " << resAvailConfig->toString() << std::endl;

            int track_id = 0;
            while (remaining_coreUsage > 0) {
                // First we attempt to get a containing window
                auto conRes = resAvailConfig->containmentQuery(tw->start, tw->stop);

                if(conRes.first != TASK_NOT_FOUND){

                    auto index = conRes.first;
                    auto result = conRes.second;

                    ComputationDevice::resource_avail_windows[cores]->removeItem(index);

                    auto left_start = result->timeWindow->start;
                    auto left_finish = tw->start;

                    auto left_tw = std::make_shared<model::ResourceWindow>(left_start, left_finish, result->deviceId, result->track_id,
                                                                           result->capacity);

                    auto right_start = tw->stop;
                    auto right_finish = result->timeWindow->stop;

                    auto right_tw = std::make_shared<model::ResourceWindow>(right_start, right_finish, result->deviceId, result->track_id, result->capacity);


                    if ((right_finish - right_start) >= resAvailConfig->min_processing_time)
                        res_windows.push_back(right_tw);

                    if ((left_finish - left_start) >= resAvailConfig->min_processing_time)
                        res_windows.push_back(left_tw);
                }
                else {
                    auto windows = resAvailConfig->overlapQuery(tw->start, tw->stop);

                    if(!windows.empty()) {
                        int subtract_index = 0;
                        for (const auto& [ind, window]: windows) {
                            ComputationDevice::resource_avail_windows[cores]->removeItem(ind - subtract_index);
                            std::chrono::time_point<std::chrono::system_clock> start;
                            std::chrono::time_point<std::chrono::system_clock> stop;
                            if(tw->start <= window->timeWindow->stop && tw->start >= window->timeWindow->start){
                                start = window->timeWindow->start;
                                stop = tw->start;
                            }
                            else if(tw->stop >= window->timeWindow->start && tw->stop <= window->timeWindow->stop){
                                start = tw->stop;
                                stop = window->timeWindow->stop;
                            }

                            auto resulting_window = std::make_shared<model::ResourceWindow>(start, stop, window->deviceId, window->track_id,
                                                                                   window->capacity);
                            res_windows.push_back(resulting_window);
                            subtract_index++;
                        }
                    }
                }


//                if(index == TASK_NOT_FOUND && coreUsage < cores && taskCounter != 0) {
//                    remaining_coreUsage -= cores;
//                    std::cout << "SKIP TASK Device: " << ComputationDevice::getHostName() << " Core Usage" << coreUsage << " CONFIG " << cores << " remain " << remaining_coreUsage << std::endl;
//                    continue;
//                }


                //NEED TO VERIFY WHAT I WAS DOING HERE
                remaining_coreUsage -= cores;
                track_id++;
            }
            std::cout << "\nAFTER: "
                      << resAvailConfig->toString()
                      << std::endl;
            if(!res_windows.empty())
                ComputationDevice::resource_avail_windows[cores]->insert(res_windows);

            utils::verify_res_avail(avail_pair.second.get());
        }

    };

} // model