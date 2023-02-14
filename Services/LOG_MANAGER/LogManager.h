//
// Created by Jamie Cotter on 13/02/2023.
//

#ifndef CONTROLLER_LOGMANAGER_H
#define CONTROLLER_LOGMANAGER_H

#include <vector>
#include <cpprest/json.h>
#include "../../model/enums/LogTypes.h"

namespace services {

    class LogManager {
    public:
        LogManager();
        void add_log(enums::LogTypeEnum logType, web::json::value log);
        std::string write_log();
    private:
        std::vector<web::json::value> log_list;
        std::mutex log_list_lock;
    };
    std::string fetchEventName(enums::LogTypeEnum logTypeEnum);
} // services

#endif //CONTROLLER_LOGMANAGER_H
