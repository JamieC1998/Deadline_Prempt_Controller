//
// Created by jamiec on 9/27/22.
//

#ifndef CONTROLLER_LINKACT_H
#define CONTROLLER_LINKACT_H

#include <utility>
#include <string>

namespace model {

    class LinkAct {
        static int link_activity_counter;

    public:
        LinkAct(bool isMeta, const std::pair<int, int> &devIds, const std::pair<std::string, std::string> &hostNames,
                float dataSize, time_t transferTime, const std::pair<time_t, time_t> &startFinTime);

        static int getLinkActivityId();

        static void setLinkActivityId(int linkActivityId);

        bool isMeta() const;

        void setIsMeta(bool isMeta);

        const std::pair<int, int> &getDevIds() const;

        void setDevIds(const std::pair<int, int> &devIds);

        const std::pair<std::string, std::string> &getHostNames() const;

        void setHostNames(const std::pair<std::string, std::string> &hostNames);

        float getDataSize() const;

        void setDataSize(float dataSize);

        time_t getTransferTime() const;

        void setTransferTime(time_t transferTime);

        const std::pair<time_t, time_t> &getStartFinTime() const;

        void setStartFinTime(const std::pair<time_t, time_t> &startFinTime);

    private:
        int link_activity_id;
        bool is_meta;
        std::pair<int, int> dev_ids;
        std::pair<std::string, std::string> host_names;

        float data_size;
        time_t transfer_time;
        std::pair<time_t, time_t> start_fin_time;
    };

} // model

#endif //CONTROLLER_LINKACT_H
