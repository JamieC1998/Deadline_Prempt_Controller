//
// Created by Jamie Cotter on 13/02/2023.
//

#include "LogManager.h"

#include <utility>
#include <thread>
#include "../../Constants/LOG_CONSTANT.h"
#include <filesystem>
#include <unistd.h>

namespace services {
    LogManager::LogManager() = default;

    double file_size = 0;
    bool stop_writing = false;

    double getStringSizeMB(const std::string &str) {
        return static_cast<double>(str.size()) / (1024.0 * 1024.0); // Convert bytes to MB
    }

    void LogManager::add_log(enums::LogTypeEnum logType, web::json::value log) {
//        std::thread([logType, log = std::move(log), this]() {
            auto log_time = std::chrono::system_clock::now();
            std::unique_lock<std::mutex> logLock(LogManager::log_list_lock, std::defer_lock);
            logLock.lock();

            uint64_t time = std::chrono::duration_cast<std::chrono::milliseconds>(log_time.time_since_epoch()).count();

            web::json::value result_json_obj;
            result_json_obj["event_type"] = web::json::value::string(fetchEventName(logType));
            result_json_obj["time"] = web::json::value::number(time);
            result_json_obj["message_content"] = web::json::value(log);

            auto serialized_obj = result_json_obj.serialize();
            log_list.push_back(serialized_obj);

            std::cout << serialized_obj << std::endl;

//            auto stringSizeMB = getStringSizeMB(serialized_obj);
//            file_size += stringSizeMB;
//
//            if (file_size > MAX_LOGGER_SIZE) {
//                std::thread([this]() {
//                    LogManager::write_log();
//                }).detach();
//            }

            logLock.unlock();

//        }).detach();
    }

    void ensureDirectoryExists(const std::string &path) {
        if (!std::filesystem::exists(path)) {
            std::filesystem::create_directories(path);
        }
    }

    void LogManager::write_log() {
        std::unique_lock<std::mutex> logLock(LogManager::log_list_lock, std::defer_lock);
        logLock.lock();

        if (stop_writing) {
            logLock.unlock();
            return;
        }

        ensureDirectoryExists(RESULTS_DIRECTORY);

        bool file_exists = std::filesystem::exists(RESULTS_DIRECTORY + "/" + RESULTS_FILE_NAME);

        std::ofstream file;

        if (!file_exists)
            file.open(RESULTS_DIRECTORY + "/" + RESULTS_FILE_NAME);
        else
            file.open(RESULTS_DIRECTORY + "/" + RESULTS_FILE_NAME, std::ios::app);

        if (file.is_open()) {
            if (!file_exists)
                file << "[";
            for (const auto &event: log_list) {
                file << event << "," << "\n";
            }
            file.close();
        }

        LogManager::log_list.clear();
        file_size = 0;
        logLock.unlock();
    }
    // services

    void LogManager::close_log() {
        std::unique_lock<std::mutex> logLock(LogManager::log_list_lock, std::defer_lock);
        logLock.lock();
        std::ofstream file;

        bool begin = false;

        ensureDirectoryExists(RESULTS_DIRECTORY);

        if (std::filesystem::exists(RESULTS_DIRECTORY + "/" + RESULTS_FILE_NAME))
            file.open(RESULTS_DIRECTORY + "/" + RESULTS_FILE_NAME, std::ios::app);
        else {
            file.open(RESULTS_DIRECTORY + "/" + RESULTS_FILE_NAME);
            begin = true;
        }

        if (file.is_open()) {
            if(begin)
                file << "[";
            for (int i = 0; i < log_list.size(); i++) {
                if(i == log_list.size() - 1)
                    file << log_list[i];
                else
                    file << log_list[i] << "," << "\n";
            }

            file << "]";
            file.close();
        }

        stop_writing = true;

        logLock.unlock();
    }

    std::string fetchEventName(enums::LogTypeEnum logTypeEnum) {
        switch (logTypeEnum) {
            case enums::LogTypeEnum::LOW_COMP_REQUEST:
                return "LOW_COMP_REQUEST";
            case enums::LogTypeEnum::HIGH_COMP_REQUEST:
                return "HIGH_COMP_REQUEST";
            case enums::LogTypeEnum::DEVICE_REGISTER:
                return "DEVICE_REGISTER";
            case enums::LogTypeEnum::IPERF_RESULTS:
                return "IPERF_RESULT";
            case enums::LogTypeEnum::DAG_DISRUPTION_REQUEST:
                return "DAG_DISRUPTION_REQUEST";
            case enums::LogTypeEnum::STATE_UPDATE_REQUEST:
                return "STATE_UPDATE_REQUEST";
            case enums::LogTypeEnum::HIGH_COMP_FINISH:
                return "HIGH_COMP_FINISH";
            case enums::LogTypeEnum::LOW_COMP_FINISH:
                return "LOW_COMP_FINISH";
            case enums::LogTypeEnum::LOW_COMP_ALLOCATION_FAIL:
                return "LOW_COMP_ALLOCATION_FAIL";
            case enums::LogTypeEnum::LOW_COMP_PREEMPTION:
                return "LOW_COMP_ALLOCATION_PREEMPTION";
            case enums::LogTypeEnum::LOW_COMP_ALLOCATION_SUCCESS:
                return "LOW_COMP_ALLOCATION_SUCCESS";
            case enums::LogTypeEnum::LOW_COMP_PREMPT_ALLOCATION_SUCCESS:
                return "LOW_COMP_PREMPT_ALLOCATION_SUCCESS";
            case enums::LogTypeEnum::HALT_REQUEST:
                return "HALT_REQUEST";
            case enums::LogTypeEnum::HIGH_COMP_ALLOCATION_CORE_FAIL:
                return "HIGH_COMP_ALLOCATION_CORE_FAIL";
            case enums::LogTypeEnum::HIGH_COMP_ALLOCATION_SUCCESS:
                return "HIGH_COMP_ALLOCATION_SUCCESS";
            case enums::LogTypeEnum::HIGH_COMP_ALLOCATION_FAIL:
                return "HIGH_COMP_ALLOCATION_FAIL";
            case enums::LogTypeEnum::HIGH_COMP_REALLOCATION_FAIL:
                return "HIGH_COMP_REALLOCATION_FAIL";
            case enums::LogTypeEnum::DAG_DISRUPTION_SUCCESS:
                return "DAG_DISRUPTION_SUCCESS";
            case enums::LogTypeEnum::DAG_DISRUPTION_FAIL:
                return "DAG_DISRUPTION_FAIL";
            case enums::LogTypeEnum::HALT_DNN:
                return "HALT_DNN";
            case enums::LogTypeEnum::ADD_WORK_TASK:
                return "ADD_WORK_TASK";
            case enums::LogTypeEnum::EXPERIMENT_INITIALISE:
                return "EXPERIMENT_INITIALISE";
            case enums::LogTypeEnum::OUTBOUND_STATE_UPDATE:
                return "OUTBOUND_STATE_UPDATE";
            case enums::LogTypeEnum::OUTBOUND_TASK_ALLOCATION_HIGH:
                return "OUTBOUND_TASK_ALLOCATION_HIGH";
            case enums::LogTypeEnum::OUTBOUND_TASK_REALLOCATION_HIGH:
                return "OUTBOUND_TASK_REALLOCATION_HIGH";
            case enums::LogTypeEnum::OUTBOUND_HALT_REQUEST:
                return "OUTBOUND_HALT_REQUEST";
            case enums::LogTypeEnum::OUTBOUND_LOW_COMP_ALLOCATION:
                return "OUTBOUND_LOW_COMP_ALLOCATION";
            case enums::LogTypeEnum::ADD_NETWORK_TASK:
                return "ADD_NETWORK_TASK";
            case enums::LogTypeEnum::HIGH_COMP_REALLOCATION_SUCCESS:
                return "HIGH_COMP_REALLOCATION_SUCCESS";
            case enums::LogTypeEnum::VIOLATED_DEADLINE:
                return "VIOLATED_DEADLINE";
            case enums::LogTypeEnum::VIOLATED_DEADLINE_REQUEST:
                return "VIOLATED_DEADLINE_REQUEST";
            case enums::LogTypeEnum::BANDWIDTH_UPDATE_LOG:
                return "CLIENT_BANDWIDTH_UPDATE";
            case enums::LogTypeEnum::BANDWIDTH_NEW_VALUE:
                return "BANDWIDTH_NEW_VALUE";
            case enums::LogTypeEnum::NEW_BUCKET_LINK:
                return "NEW_BUCKET_LINK";
            case enums::LogTypeEnum::OUTBOUND_COMMS_TEST:
                return "OUTBOUND_COMMS_TEST";
        }
        return "NaN";
    }
}