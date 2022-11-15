//
// Created by Jamie Cotter on 25/10/2022.
//

#include <iomanip>
#include "UtilFunctions.h"
#include "cpprest/json.h"
#include "../../Constants/CLIENT_DETAILS.h"

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

    unsigned long calculateSizeOfInputData(std::shared_ptr<model::BaseDNNModel> bDNN) {

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
        output["current_block"] = web::json::value(std::move(temp_current_block));

        auto group_blocks_val = web::json::value();

        std::vector<enums::LayerTypeEnum> is_conv;

        for(int i = 0; i < bDNN->getLayerType().size(); i++){
            if(bDNN->getLayerType()[i] == enums::LayerTypeEnum::pooling)
                is_conv.push_back(enums::LayerTypeEnum::pooling);
            else if(i + 1 < bDNN->getLayerType().size() && bDNN->getLayerType()[i + 1] == enums::LayerTypeEnum::pooling)
                is_conv.push_back(enums::LayerTypeEnum::conv);
            else
                is_conv.push_back(enums::LayerTypeEnum::conv);
        }

        for (int i = 0; i < is_conv.size(); i++) {
            auto group_block = web::json::value();
            //DECIDING MAX PARTITION COUNT
            for (int j = 0; j < (is_conv[i] == enums::LayerTypeEnum::conv) ? MAX_CORES : 1; j++) {
                auto block = web::json::value();
                block["estimated_finish_time"] = web::json::value(utils::convertDateToString(std::chrono::high_resolution_clock::now()));
                block["estimated_start_time"] = web::json::value(utils::convertDateToString(std::chrono::high_resolution_clock::now()));
                block["group_block_id"] = web::json::value(INT_MAX);
                block["block_id"] = web::json::value(INT_MAX);
                block["allocated_device"] = web::json::value(tmp_string);

                auto inner_layer_ids = std::vector<web::json::value>();

                for (int c = 0; c < bDNN->getLayerType().size(); c++)
                    inner_layer_ids.emplace_back(INT_MAX);

                block["original_layer_ids"] = web::json::value::array(inner_layer_ids);

                auto input_tile_region = web::json::value();
                input_tile_region["x1"] = INT_MAX;
                input_tile_region["x2"] = INT_MAX;
                input_tile_region["y1"] = INT_MAX;
                input_tile_region["y2"] = INT_MAX;
                block["in_map"] = input_tile_region;

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
} // utils