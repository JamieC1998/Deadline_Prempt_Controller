//
// Created by Jamie Cotter on 25/10/2022.
//

#include <iomanip>
#include "UtilFunctions.h"
#include "cpprest/json.h"
#include "../../Constants/CLIENT_DETAILS.h"
#include "../../Constants/FTP_CONFIG.h"
#include "../../model/data_models/ComputationDevice/ComputationDevice.h"

namespace utils {
    //Credit to the anonymous user who posted this https://www.mycompiler.io/view/43wsMbrMcmx
    std::string convertDateToString(std::chrono::time_point<std::chrono::high_resolution_clock> timePoint) {
        const std::chrono::high_resolution_clock::time_point::duration tt = timePoint.time_since_epoch();
        const time_t durS = std::chrono::duration_cast<std::chrono::seconds>(tt).count();
        std::ostringstream ss;
        const std::tm *tm = std::gmtime(&durS);
        ss << std::put_time(tm, "%Y-%m-%d %H:%M:%S.");

        const long long durMs = std::chrono::duration_cast<std::chrono::milliseconds>(tt).count();
        ss << std::setw(3) << std::setfill('0') << int(durMs - durS * 1000);
        return ss.str();
    }

    std::ifstream::pos_type filesize(std::string filename)
    {
        std::ifstream in(filename, std::ifstream::ate | std::ifstream::binary);
        return in.tellg();
    }

    std::string debugTimePointToString(const std::chrono::system_clock::time_point& tp)
    {
        const char* format = "%Y-%m-%d %H:%M:%S.%f";
        std::time_t t = std::chrono::system_clock::to_time_t(tp);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch()) % 1000;
        std::tm tm = *std::localtime(&t);
        std::stringstream ss;
        ss << std::put_time(&tm, format) << "." << std::setfill('0') << std::setw(3) << ms.count();
        return ss.str();
    }

    bool verify_res_avail(model::ResourceAvailabilityList* resourceAvailabilityList) {
        std::vector<int> unique_ids = {};
        for(const auto & resource_window : resourceAvailabilityList->resource_windows){
            bool insert = true;
            for(int unique_id : unique_ids){
                if(unique_id == resource_window->track_id){
                    insert = false;
                }
            }
            if(insert && resource_window->timeWindow->stop == std::chrono::system_clock::time_point::max())
                unique_ids.push_back(resource_window->track_id);
        }
        if(unique_ids.size() != resourceAvailabilityList->device_track_count) {
            std::cout << "";
            return false;
        }
        return true;
    }

    std::string request_type_parser(enums::request_type requestType){
        switch (static_cast<int>(requestType)) {
            case REQ_TYPE_STATE_UPDATE:
                return "REQ_TYPE_STATE_UPDATE";
            case REGENERATE_DATA_STRUCTURE:
                return "REGENERATE_DATA_STRUCTURE";
            case BANDWIDTH_UPDATE_ITEM:
                return "BANDWIDTH_UPDATE_ITEM";
            case NETWORK_DISC_UPDATE:
                return "NETWORK_DISC_UPDATE";
            case HALT_REQ:
                return "HALT_REQ";
            case LOW_COMPLEXITY:
                return "LOW_COMPLEXITY";
            case HIGH_COMPLEXITY:
                return "HIGH_COMPLEXITY";
        }
    }
} // utils

