//
// Created by jamiec on 9/27/22.
//

#ifndef CONTROLLER_LINKACT_H
#define CONTROLLER_LINKACT_H

#include <utility>
#include <string>
#include <chrono>
#include "cpprest/json.h"

namespace model {

    class LinkAct {
        static int link_activity_counter;

    public:
        LinkAct(bool isMeta, std::pair<std::string, std::string> &hostNames,
                uint64_t dataSize, std::pair<std::chrono::time_point<std::chrono::system_clock>, std::chrono::time_point<std::chrono::system_clock>> &startFinTime);

        LinkAct();

        explicit LinkAct(
                std::pair<std::chrono::time_point<std::chrono::system_clock>, std::chrono::time_point<std::chrono::system_clock>> startFinTime);

        int getLinkActivityId();

        void setLinkActivityId(int linkActivityId);

        bool isMeta() const;

        void setIsMeta(bool isMeta);

        const std::pair<std::string, std::string> &getHostNames() const;

        void setHostNames(const std::pair<std::string, std::string> &hostNames);

        uint64_t getDataSize() const;

        void setDataSize(uint64_t dataSize);

        const std::pair<std::chrono::time_point<std::chrono::system_clock>, std::chrono::time_point<std::chrono::system_clock>> &getStartFinTime() const;

        void setStartFinTime(const std::pair<std::chrono::time_point<std::chrono::system_clock>, std::chrono::time_point<std::chrono::system_clock>> &startFinTime);

        web::json::value convertToJson();

        const std::pair<std::chrono::time_point<std::chrono::system_clock>, std::chrono::time_point<std::chrono::system_clock>> &
        getActualStartFinTime() const;

        void setActualStartFinTime(
                const std::pair<std::chrono::time_point<std::chrono::system_clock>, std::chrono::time_point<std::chrono::system_clock>> &actualStartFinTime);

    private:
        int link_activity_id;
        bool is_meta;
        std::pair<std::string, std::string> host_names;

        uint64_t data_size;
        std::pair<std::chrono::time_point<std::chrono::system_clock>, std::chrono::time_point<std::chrono::system_clock>> start_fin_time;

        std::pair<std::chrono::time_point<std::chrono::system_clock>, std::chrono::time_point<std::chrono::system_clock>> actual_start_fin_time = std::make_pair(std::chrono::time_point<std::chrono::system_clock>(
                std::chrono::milliseconds{0}), std::chrono::time_point<std::chrono::system_clock>(
                std::chrono::milliseconds{0}));
    };

} // model

#endif //CONTROLLER_LINKACT_H
