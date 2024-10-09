//
// Created by jamiec on 9/27/22.
//

#include "ComputationDevice.h"
#include "../CompResult/HighCompResult/HighCompResult.h"
#include "../CompResult/LowCompResult/LowCompResult.h"
#include "../../../Constants/AllocationMacros.h"
#include "../../../Constants/FTP_CONFIG.h"

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
            cores), host_name(std::move(hostName)), id(id_counter) {
        id_counter++;
        std::chrono::time_point<std::chrono::system_clock> start_time = std::chrono::system_clock::now();

        ComputationDevice::resource_avail_windows[LOW_COMPLEXITY_CORE_COUNT] = std::make_shared<model::ResourceAvailabilityList>(
                LOW_COMPLEXITY_CORE_COUNT, cores / LOW_COMPLEXITY_CORE_COUNT, cores,
                std::chrono::milliseconds{LOW_COMPLEXITY_PROCESSING_TIME}, start_time, hostName);
        ComputationDevice::resource_avail_windows[FTP_LOW_CORE] = std::make_shared<model::ResourceAvailabilityList>(
                FTP_LOW_CORE,
                cores /
                FTP_LOW_CORE,
                cores,
                std::chrono::milliseconds{
                        FTP_LOW_TIME},
                start_time,
                hostName);
        ComputationDevice::resource_avail_windows[FTP_HIGH_CORE] = std::make_shared<model::ResourceAvailabilityList>(
                FTP_HIGH_CORE,
                cores /
                FTP_HIGH_CORE,
                cores,
                std::chrono::milliseconds{
                        FTP_HIGH_TIME},
                start_time,
                hostName);

    }

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

    void ComputationDevice::resAvailRemoveAndSplit(std::shared_ptr<model::TimeWindow> tw, int coreUsage) {

        for (auto [cores, resAvailConfig]: ComputationDevice::resource_avail_windows) {
            int remaining_coreUsage = coreUsage;
            std::vector<std::shared_ptr<model::ResourceWindow>> res_windows;

            while (remaining_coreUsage > 0) {
                auto [index, result] = resAvailConfig->containmentQuery(tw->start, tw->stop);
                resAvailConfig->removeItem(index);

                auto left_start = result->timeWindow->start;
                auto left_finish = tw->start;

                auto left_tw = std::make_shared<model::ResourceWindow>(left_start, left_finish, result->deviceId,
                                                                       result->capacity);

                auto right_start = tw->stop;
                auto right_finish = result->timeWindow->stop;

                auto right_tw = std::make_shared<model::ResourceWindow>(right_start, right_finish, result->deviceId,
                                                                        result->capacity);


                if ((right_finish - right_start) >= resAvailConfig->min_processing_time)
                    res_windows.push_back(right_tw);

                if ((left_finish - left_start) >= resAvailConfig->min_processing_time)
                    res_windows.push_back(left_tw);

                remaining_coreUsage - cores;
            }
            resAvailConfig->insert(res_windows);
        }

    };

} // model