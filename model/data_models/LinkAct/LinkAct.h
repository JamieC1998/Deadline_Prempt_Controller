//
// Created by jamiec on 9/27/22.
//

#ifndef CONTROLLER_LINKACT_H
#define CONTROLLER_LINKACT_H

#include <utility>
#include <string>
#include <chrono>

namespace model {

    class LinkAct {
        static int link_activity_counter;

    public:
        LinkAct(bool isMeta, const std::pair<int, int> &devIds, const std::pair<std::string, std::string> &hostNames,
                float dataSize, std::chrono::duration<long long int, std::ratio<1, 1000000>> transferTimeMs, const std::pair<std::chrono::time_point<std::chrono::system_clock>, std::chrono::time_point<std::chrono::system_clock>> &startFinTime);

        LinkAct();

        explicit LinkAct(
                const std::pair<std::chrono::time_point<std::chrono::system_clock>, std::chrono::time_point<std::chrono::system_clock>> &startFinTime);

        int getLinkActivityId();

        void setLinkActivityId(int linkActivityId);

        bool isMeta() const;

        void setIsMeta(bool isMeta);

        const std::pair<int, int> &getDevIds() const;

        void setDevIds(const std::pair<int, int> &devIds);

        const std::pair<std::string, std::string> &getHostNames() const;

        void setHostNames(const std::pair<std::string, std::string> &hostNames);

        float getDataSize() const;

        void setDataSize(float dataSize);

        const std::chrono::time_point<std::chrono::system_clock> getTransferTime() const;

        void setTransferTime(std::chrono::time_point<std::chrono::system_clock> transferTime);

        const std::pair<std::chrono::time_point<std::chrono::system_clock>, std::chrono::time_point<std::chrono::system_clock>> &getStartFinTime() const;

        void setStartFinTime(const std::pair<std::chrono::time_point<std::chrono::system_clock>, std::chrono::time_point<std::chrono::system_clock>> &startFinTime);

    private:
        int link_activity_id = 0;
        bool is_meta;
        std::pair<int, int> dev_ids;
        std::pair<std::string, std::string> host_names;

        float data_size;
        std::chrono::time_point<std::chrono::system_clock> transfer_time_ms;
        std::pair<std::chrono::time_point<std::chrono::system_clock>, std::chrono::time_point<std::chrono::system_clock>> start_fin_time;
    };

} // model

#endif //CONTROLLER_LINKACT_H
