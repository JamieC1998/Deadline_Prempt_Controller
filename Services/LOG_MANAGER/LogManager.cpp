//
// Created by Jamie Cotter on 13/02/2023.
//

#include "LogManager.h"

#include <utility>
#include "../../model/enums/LogTypes.h"
#include "../../Constants/LOG_CONSTANT.h"

namespace services {
    LogManager::LogManager() = default;

    void LogManager::add_log(enums::LogTypeEnum logType, web::json::value log) {
        std::unique_lock<std::mutex> logLock(LogManager::log_list_lock, std::defer_lock);
        logLock.lock();
        std::chrono::time_point<std::chrono::system_clock> log_time = std::chrono::system_clock::now();
        uint64_t time = std::chrono::duration_cast<std::chrono::milliseconds>(log_time.time_since_epoch()).count();
        web::json::value result_json_obj;
        result_json_obj["event_type"] = web::json::value::string(fetchEventName(logType));
        result_json_obj["time"] = web::json::value::number(time);
        result_json_obj["message_content"] = web::json::value(std::move(log));

        log_list.push_back(result_json_obj);

        //std::cout << result_json_obj.serialize() << std::endl;

        logLock.unlock();
    }

    std::string LogManager::write_log() {
        std::unique_lock<std::mutex> logLock(LogManager::log_list_lock, std::defer_lock);
        logLock.lock();
        web::json::value array = web::json::value::array(LogManager::log_list);
        auto serialised_result = array.serialize();
        std::ofstream file(RESULTS_FILE);
        if (file.is_open())
        {
            file << array.serialize();
            file.close();
        }

        logLock.release();
        return serialised_result;
    }
    // services

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
            case enums::LogTypeEnum::LATENCY_LOG:
                return "LATENCY_LOG";
        }
        return "NaN";
    }
}