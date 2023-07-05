//
// Created by Jamie Cotter on 03/02/2023.
//

#ifndef JSONPARSETEST_PARTITIONCONFIG_H
#define JSONPARSETEST_PARTITIONCONFIG_H

#include <cpprest/json.h>

namespace model {
    class PartitionConfig {
    public:
        PartitionConfig(web::json::value& proc_time, web::json::value& data_size);

        PartitionConfig();

        int getProcTimeMilliseconds() const;

        void setProcTimeMilliseconds(int procTimeMilliseconds);

        int getN() const;

        int getM() const;

        int getBlockCount() const;

        void setBlockCount(int blockCount);

    private:
        int proc_time_milliseconds;
        int N = 0;
        int M = 0;

        int block_count = 0;
    };
}

#endif //JSONPARSETEST_PARTITIONCONFIG_H
