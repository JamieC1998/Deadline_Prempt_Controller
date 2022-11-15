//
// Created by jamiec on 9/27/22.
//

#include "LinkAct.h"

namespace model {
    int LinkAct::link_activity_counter = 0;

    LinkAct::LinkAct(bool isMeta, std::pair<int, int> &devIds,
                     std::pair<std::string, std::string> &hostNames, float dataSize,
                     std::pair<std::chrono::time_point<std::chrono::high_resolution_clock>, std::chrono::time_point<std::chrono::high_resolution_clock>> &startFinTime) : is_meta(isMeta), dev_ids(devIds),
                                                                      host_names(hostNames), data_size(dataSize),
                                                                      start_fin_time(startFinTime), link_activity_id(
                    link_activity_counter) { link_activity_counter++; }

    int LinkAct::getLinkActivityId() {
        return link_activity_id;
    }

    void LinkAct::setLinkActivityId(int linkActivityId) {
        link_activity_id = linkActivityId;
    }

    bool LinkAct::isMeta() const {
        return is_meta;
    }

    void LinkAct::setIsMeta(bool isMeta) {
        is_meta = isMeta;
    }

    const std::pair<int, int> &LinkAct::getDevIds() const {
        return dev_ids;
    }

    void LinkAct::setDevIds(const std::pair<int, int> &devIds) {
        dev_ids = devIds;
    }

    const std::pair<std::string, std::string> &LinkAct::getHostNames() const {
        return host_names;
    }

    void LinkAct::setHostNames(const std::pair<std::string, std::string> &hostNames) {
        host_names = hostNames;
    }

    float LinkAct::getDataSize() const {
        return data_size;
    }

    void LinkAct::setDataSize(float dataSize) {
        data_size = dataSize;
    }

    const std::pair<std::chrono::time_point<std::chrono::high_resolution_clock>, std::chrono::time_point<std::chrono::high_resolution_clock>> &LinkAct::getStartFinTime() const {
        return start_fin_time;
    }

    void LinkAct::setStartFinTime(const std::pair<std::chrono::time_point<std::chrono::high_resolution_clock>, std::chrono::time_point<std::chrono::high_resolution_clock>> &startFinTime) {
        start_fin_time = startFinTime;
    }

    LinkAct::LinkAct() {}

    LinkAct::LinkAct(
            const std::pair<std::chrono::time_point<std::chrono::high_resolution_clock>, std::chrono::time_point<std::chrono::high_resolution_clock>> &startFinTime)
            : start_fin_time(startFinTime) {}
} // model