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

    void sortLink(std::vector<std::shared_ptr<model::LinkAct>>* network_link) {
        std::sort(network_link->begin(), network_link->end(),
                  [](const std::shared_ptr<model::LinkAct> &a, const std::shared_ptr<model::LinkAct> &b) {
                      return a->start_fin_time->stop < b->start_fin_time->stop;
                  });
    }

    unsigned long calculateStateUpdateSize() {
        web::json::value output;
        output["type"] = web::json::value(true);
        output["task_id"] = web::json::value(INT_MAX);

        return std::string(output.serialize()).length() * sizeof(char);
    }

    /* Function calculates the N * M position of a task given its flatteened index */
    std::pair<int, int> fetchN_M(int position, int width, int height) {
        position = position + 1;
        int counter = 1;
        int N = 1;
        int M = 1;
        for(int i = 1; i <= height; i++){
            M = i;
            for(int j = 1; j <= width; j++){
                N = j;
                if(counter == position)
                    return std::make_pair(N, M);
                counter++;
            }

        }
        return std::make_pair(N, M);
    }

    bool is_allocated(std::string task_id, std::vector<std::string> completed_tasks){
        for(const auto& comp_id: completed_tasks){
            if(task_id == comp_id)
                return true;
        }
        return false;
    }

    std::map<std::string, int> generateAllocationMap(std::map<std::string, std::shared_ptr<model::ComputationDevice>> devices){
        std::map<std::string, int> alloMap;

        for(const auto& [device_id, device]: devices){
            alloMap[device_id] = 0;
        }

        return alloMap;
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
} // utils

