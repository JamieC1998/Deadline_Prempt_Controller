//
// Created by Jamie Cotter on 03/02/2023.
//

#include "PartitionConfig.h"

model::PartitionConfig::PartitionConfig(web::json::value &proc_time, web::json::value &data_size)
        : proc_time_milliseconds(
        static_cast<int>(ceil(((proc_time["mean"].as_double() + proc_time["stdev"].as_double()) * 1000)))),
          N(data_size["N"].as_integer()), M(data_size["M"].as_integer()) {

    for (auto &property: data_size["per_tile"].as_array()) {
        PartitionConfig::tile_size.push_back(property.as_integer());
    }
}

int model::PartitionConfig::getProcTimeMilliseconds() const {
    return proc_time_milliseconds;
}

int model::PartitionConfig::getN() const {
    return N;
}

int model::PartitionConfig::getM() const {
    return M;
}

const std::vector<uint64_t> &model::PartitionConfig::getTileSize() const {
    return tile_size;
}

model::PartitionConfig::PartitionConfig() = default;


