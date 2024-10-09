//
// Created by jamiec on 9/27/22.
//

#ifndef CONTROLLER_LINKACT_H
#define CONTROLLER_LINKACT_H

#include <utility>
#include <string>
#include <chrono>
#include "cpprest/json.h"
#include "../TimeWindow/TimeWindow.h"

namespace model {

    class LinkAct {
        static int link_activity_counter;

    public:
        std::shared_ptr<TimeWindow> start_fin_time;

        std::shared_ptr<TimeWindow> actual_start_fin_time;

        LinkAct(bool isMeta, std::pair<std::string, std::string> &hostNames,
                uint64_t dataSize, std::chrono::time_point<std::chrono::system_clock> start, std::chrono::time_point<std::chrono::system_clock> fin);

        LinkAct();

        explicit LinkAct(
                std::chrono::time_point<std::chrono::system_clock> start, std::chrono::time_point<std::chrono::system_clock> fin);

        int getLinkActivityId();

        void setLinkActivityId(int linkActivityId);

        bool isMeta() const;

        void setIsMeta(bool isMeta);

        const std::pair<std::string, std::string> &getHostNames() const;

        void setHostNames(const std::pair<std::string, std::string> &hostNames);

        uint64_t getDataSize() const;

        void setDataSize(uint64_t dataSize);

        void setStartFinTime(const std::pair<std::chrono::time_point<std::chrono::system_clock>, std::chrono::time_point<std::chrono::system_clock>> &startFinTime);

        web::json::value convertToJson();

    private:
        int link_activity_id;
        bool is_meta;
        std::pair<std::string, std::string> host_names;

        uint64_t data_size;
    };

} // model

#endif //CONTROLLER_LINKACT_H
