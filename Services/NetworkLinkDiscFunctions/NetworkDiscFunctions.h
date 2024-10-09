//
// Created by Jamie Cotter on 02/10/2024.
//

#ifndef CONTROLLER_NETWORKDISCFUNCTIONS_H
#define CONTROLLER_NETWORKDISCFUNCTIONS_H

#include <vector>
#include <cmath>  // For log2 and floor functions
#include "../../model/data_models/Bucket/Bucket.h"

namespace services {
    /* Input:
     * time_val - current time
     * base_comm_size_ms - The smallest bucket size
     * number_of_base_buckets - the number of single sized buckets before logarithmic growth
     *
     * This function takes in a timestamp and returns back its appropriate index on the network link
     * Returns:
     * An integer index */
    uint64_t ms_to_index(uint64_t time_val, uint64_t base_comm_size_ms, int number_of_base_buckets);

    /* A wrapper function that ensures that for any net_link index
     * we try to obtain that it is rounded up to the next bucket,
     * to avoid inserting into an on_going bucket.
     * As well as converting a given timestamp to an 0th offset for our list. */
    uint64_t obtain_index(uint64_t time, uint64_t start_time_offset, uint64_t base_comm_size_ms, int number_of_base_buckets);

    /* Input:
     * current_time_of_reasoning - current time
     * number_of_base_buckets - the index where the size of the buckets begins doubling
     * net_link - A list of buckets that represent our network link, contains a time window, bucket size and bucket contents
     *  base_comm_size_ms - the expected size of communication
     *
     * Given an input network link and the current time, the function creates
     * a new network link with communication downshifted to appropriate buckets
     * based on the passage of time.
     *
     * Returns:
     * new_net_link - A new network link that contains a subset of the communication items from the input link */
    std::vector<std::shared_ptr<model::Bucket>> cascade_function(std::chrono::time_point<std::chrono::system_clock> current_time_of_reasoning, int number_of_base_buckets, std::vector<std::shared_ptr<model::Bucket>> net_link, uint64_t base_comm_size_ms, int exp_bucket_count);
}


#endif //CONTROLLER_NETWORKDISCFUNCTIONS_H
