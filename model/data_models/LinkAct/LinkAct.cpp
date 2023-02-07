//
// Created by jamiec on 9/27/22.
//

#include "LinkAct.h"

#include <utility>

namespace model {
    int LinkAct::link_activity_counter = 0;

    LinkAct::LinkAct(bool isMeta,
                     std::pair<std::string, std::string> &hostNames, uint64_t dataSize,
                     std::pair<std::chrono::time_point<std::chrono::system_clock>, std::chrono::time_point<std::chrono::system_clock>> &startFinTime)
            : is_meta(isMeta),
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

    const std::pair<std::string, std::string> &LinkAct::getHostNames() const {
        return host_names;
    }

    void LinkAct::setHostNames(const std::pair<std::string, std::string> &hostNames) {
        host_names = hostNames;
    }

    uint64_t LinkAct::getDataSize() const {
        return data_size;
    }

    void LinkAct::setDataSize(uint64_t dataSize) {
        data_size = dataSize;
    }

    const std::pair<std::chrono::time_point<std::chrono::system_clock>, std::chrono::time_point<std::chrono::system_clock>> &
    LinkAct::getStartFinTime() const {
        return start_fin_time;
    }

    void LinkAct::setStartFinTime(
            const std::pair<std::chrono::time_point<std::chrono::system_clock>, std::chrono::time_point<std::chrono::system_clock>> &startFinTime) {
        start_fin_time = startFinTime;
    }

    LinkAct::LinkAct() {}

    LinkAct::LinkAct(
            std::pair<std::chrono::time_point<std::chrono::system_clock>, std::chrono::time_point<std::chrono::system_clock>> startFinTime)
            : start_fin_time(std::move(startFinTime)) {}

    web::json::value LinkAct::convertToJson() {
        web::json::value linkActJson;

        linkActJson["link_activity_id"] = web::json::value::number(LinkAct::getLinkActivityId());
        linkActJson["is_meta"] = web::json::value::boolean(LinkAct::isMeta());
        auto hostNames = LinkAct::getHostNames();
        linkActJson["host_names"]["first"] = web::json::value::string(hostNames.first);
        linkActJson["host_names"]["second"] = web::json::value::string(hostNames.second);
        linkActJson["data_size"] = web::json::value::number(LinkAct::getDataSize());
        auto startFinTime = getStartFinTime();
        linkActJson["start_fin_time"]["first"] = web::json::value::number(std::chrono::duration_cast<std::chrono::milliseconds>(
                startFinTime.first.time_since_epoch()).count());
        linkActJson["start_fin_time"]["second"] = web::json::value::number(std::chrono::duration_cast<std::chrono::milliseconds>(
                startFinTime.second.time_since_epoch()).count());
        return linkActJson;
    }
} // model