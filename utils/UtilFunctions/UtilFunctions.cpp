//
// Created by Jamie Cotter on 25/10/2022.
//

#include <iomanip>
#include "UtilFunctions.h"
#include "cpprest/json.h"
#include "../../Constants/CLIENT_DETAILS.h"
#include "../../model/data_models/FTP_Lookup/FTP_Lookup.h"
#include "../../Constants/FTP_CONFIG.h"

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
                      return a->getStartFinTime().second < b->getStartFinTime().second;
                  });
    }

    uint64_t calculateSizeOfInputData(std::shared_ptr<model::FTP_Lookup> bDNN) {

        web::json::value output;
        output["type"] = web::json::value(true);

        //MAX SIZE OF AN IPV4 ADDRESS IS 15 characters
        std::string tmp_string = "";

        for(int i = 0; i < 15; i++)
            tmp_string += "1";

        //MAX SIZE OF AN IPV4 ADDRESS IS 15 characters


        output["source"] = web::json::value(tmp_string);
        output["deadline"] = web::json::value(
                utils::convertDateToString((std::chrono::high_resolution_clock::now())));
        output["estimated_start_time"] = web::json::value(
                utils::convertDateToString(std::chrono::high_resolution_clock::now()));
        output["estimated_start_time"] = web::json::value(
                utils::convertDateToString(std::chrono::high_resolution_clock::now()));
        output["estimated_finish_time"] = web::json::value(
                utils::convertDateToString(std::chrono::high_resolution_clock::now()));
        output["dnn_id"] = web::json::value(INT_MAX);

        std::string temp_current_block = std::to_string(INT_MAX) + "_" + std::to_string(INT_MAX);
        output["current_convidx"] = web::json::value(static_cast<int>(bDNN->partition_setup.size()));
        output["current_block"] = MAX_CORES;

        auto group_blocks_val = web::json::value();

        for (int i = 0; i < bDNN->partition_setup.size(); i++) {
            auto group_block = web::json::value();
            //DECIDING MAX PARTITION COUNT
            for (int j = 0; j < MAX_CORES; j++) {
                auto block = web::json::value();
                block["estimated_finish_time"] = web::json::value(utils::convertDateToString(std::chrono::high_resolution_clock::now()));
                block["estimated_start_time"] = web::json::value(utils::convertDateToString(std::chrono::high_resolution_clock::now()));
                block["convidx"] = web::json::value(static_cast<int>(bDNN->partition_setup.size()));
                block["convidx"] = web::json::value(INT_MAX);
                block["allocated_device"] = web::json::value(tmp_string);

                block["N"] = MAX_CORES;
                block["M"] = MAX_CORES;

                group_block[j] = block;
            }
            group_blocks_val[i] = group_block;
        }

        return std::string(output.serialize()).length() * sizeof(char);
    }

    unsigned long calculateStateUpdateSize() {
        web::json::value output;
        output["type"] = web::json::value(true);
        output["task_id"] = web::json::value(INT_MAX);

        return std::string(output.serialize()).length() * sizeof(char);
    }

    /* Function calculates the N * M position of a task given its flatteened index */
    std::pair<int, int> fetchN_M(int position, int width, int height) {
        int counter = 0;
        int N = 0;
        int M = 0;
        for(int i = 1; i == height; i++){
            M = i;
            for(int j = 1; j == width; j++){
                N = j;
                counter++;
                if(counter == position)
                    return std::make_pair(N, M);
            }
        }
    }

    std::shared_ptr<model::FTP_Lookup> parseFTP_Lookup(){
        std::ifstream proc_file(FTP_PROCESSING_TIME);
        std::ifstream data_file(FTP_DATA_SIZE);

        web::json::value proc_times;
        web::json::value data_size;

        try {
            proc_times = web::json::value::parse(proc_file);
            data_size = web::json::value::parse((data_file));
        } catch (const web::json::json_exception &e) {
            std::cout << "Error parsing JSON: " << e.what() << std::endl;
            exit(1);
        }


        return std::make_shared<model::FTP_Lookup>(model::FTP_Lookup(proc_times, data_size));
    }
} // utils

