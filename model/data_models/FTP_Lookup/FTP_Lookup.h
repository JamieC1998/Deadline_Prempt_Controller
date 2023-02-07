//
// Created by Jamie Cotter on 03/02/2023.
//

#ifndef JSONPARSETEST_FTP_LOOKUP_H
#define JSONPARSETEST_FTP_LOOKUP_H

#include <map>
#include <cpprest/json.h>
#include "../PartitionConfig/PartitionConfig.h"

namespace model {
    class FTP_Lookup {
    public:
        FTP_Lookup(web::json::value proc_times, web::json::value data_sizes);
        //TOP LEVEL KEY, CONVIDX
        //SECONDARY KEY CORE COUNT
        std::map<std::string, std::map<std::string, PartitionConfig>> partition_setup;
    };
}

#endif //JSONPARSETEST_FTP_LOOKUP_H
