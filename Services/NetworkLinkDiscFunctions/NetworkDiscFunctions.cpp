//
// Created by Jamie Cotter on 02/10/2024.
//

#include "NetworkDiscFunctions.h"

namespace services {

    uint64_t ms_to_index(uint64_t time_val, uint64_t base_comm_size_ms, int number_of_base_buckets) {
        // Round to the nearest multiple of base_comm_size_ms
        time_val = time_val + (base_comm_size_ms - (time_val % base_comm_size_ms));
        uint64_t index = time_val / base_comm_size_ms;

        // If it falls into a bucket less than our logarithmic buckets
        if (index < number_of_base_buckets) {
            return static_cast<uint64_t>(std::floor(index));
        }

        // If it's a growing bucket
        return static_cast<uint64_t>(std::floor(std::log2(index) + 2));
    }

    uint64_t obtain_index(uint64_t time, uint64_t start_time_offset, uint64_t base_comm_size_ms, int number_of_base_buckets) {
        // Call ms_to_index with adjusted time and add 1 to round up to the next bucket
        return ms_to_index((time - start_time_offset), base_comm_size_ms, number_of_base_buckets) + 1;
    }

    std::vector<std::shared_ptr<model::Bucket>> cascade_function(std::chrono::time_point<std::chrono::system_clock> current_time_of_reasoning, int number_of_base_buckets, std::vector<std::shared_ptr<model::Bucket>> net_link, uint64_t base_comm_size_ms, int exp_bucket_count) {
        std::vector<std::shared_ptr<model::Bucket>> new_net_link;

        auto current_time_reason_ms = (std::chrono::duration_cast<std::chrono::milliseconds>(current_time_of_reasoning.time_since_epoch())).count();
        auto current_time = std::chrono::time_point<std::chrono::system_clock>(std::chrono::milliseconds {current_time_reason_ms + (base_comm_size_ms - (current_time_reason_ms % base_comm_size_ms))});
        int current_bucket_size = 1;

        // Need to generate an empty new network link set to our current start time
        for (size_t i = 0; i < number_of_base_buckets + exp_bucket_count; ++i) {
            if (i >= number_of_base_buckets) {
                current_bucket_size *= 2;
            }

            new_net_link.emplace_back(std::make_shared<model::Bucket>(current_bucket_size, current_time, current_time + std::chrono::milliseconds{base_comm_size_ms * current_bucket_size}));

            current_time += std::chrono::milliseconds{base_comm_size_ms * current_bucket_size};
        }

        if(net_link.empty())
            return new_net_link;
        // Begin iterating through our old link in reverse order
        // Moving any comm items that haven't expired to an appropriate bucket
        for (int i = net_link.size() - 1; i >= 0; --i) {
            // If the complete bucket is expired, continue
            if (net_link[i]->timeWindow->stop < current_time_of_reasoning) {
                break;
            }

            // Iterate through the bucket contents and downshift
            int new_bucket_index = ms_to_index(static_cast<uint64_t>((net_link[i]->timeWindow->start - current_time_of_reasoning).count()), base_comm_size_ms, number_of_base_buckets);

            for (size_t j = 0; j < net_link[i]->bucketContents.size(); ++j) {
                if (new_net_link[new_bucket_index]->bucketContents.size() == new_net_link[new_bucket_index]->capacity) {
                    ++new_bucket_index;
                }
                new_net_link[new_bucket_index]->bucketContents.push_back(net_link[i]->bucketContents[j]);
            }
        }

        return new_net_link;
    }

}