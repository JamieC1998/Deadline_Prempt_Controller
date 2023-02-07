//
// Created by Jamie Cotter on 03/02/2023.
//

#include "FTP_Lookup.h"

model::FTP_Lookup::FTP_Lookup(web::json::value proc_times, web::json::value data_sizes) {
    for(const auto& property: proc_times.as_object()){
        auto convidx = property.first;
        auto cores_config = property.second;
        auto data_config = data_sizes[convidx];

        std::map<std::string, PartitionConfig> part_config;
        for(const auto& core_conf_details: cores_config.as_object()){
            part_config[core_conf_details.first] = PartitionConfig(cores_config[core_conf_details.first], data_config[core_conf_details.first]);
        }
        FTP_Lookup::partition_setup[convidx] = part_config;
    }
}
