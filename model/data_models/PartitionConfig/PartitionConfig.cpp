//
// Created by Jamie Cotter on 03/02/2023.
//

#include "PartitionConfig.h"

model::PartitionConfig::PartitionConfig(web::json::value &proc_time, web::json::value &data_size)
        : proc_time_milliseconds(
        static_cast<int>(ceil(((proc_time["mean"].as_double() + proc_time["stdev"].as_double()) * 1000)))),
          N(data_size["N"].as_integer()), M(data_size["M"].as_integer()), block_count(1) {

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

void model::PartitionConfig::setProcTimeMilliseconds(int procTimeMilliseconds) {
    proc_time_milliseconds = procTimeMilliseconds;
}

int model::PartitionConfig::getBlockCount() const {
    return block_count;
}

void model::PartitionConfig::setBlockCount(int blockCount) {
    block_count = blockCount;
}

model::PartitionConfig::PartitionConfig() = default;


