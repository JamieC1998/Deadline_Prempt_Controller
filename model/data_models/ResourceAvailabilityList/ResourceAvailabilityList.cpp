//
// Created by Jamie Cotter on 30/09/2024.
//

#include "ResourceAvailabilityList.h"
#include "../../../utils/UtilFunctions/UtilFunctions.h"
#include <limits>

namespace model {

    ResourceAvailabilityList::ResourceAvailabilityList(int taskResUsage, int deviceTrackCount, int deviceCoreCount,
                                                       std::chrono::milliseconds minProcessingTime,
                                                       std::chrono::time_point<std::chrono::system_clock> start_time,
                                                       std::string hname) : task_res_usage(
            taskResUsage), device_track_count(deviceTrackCount), device_core_count(deviceCoreCount),
                                                                                             min_processing_time(
                                                                                                     minProcessingTime) {
        std::string host = hname;
        for (int i = 0; i < deviceTrackCount; i++) {
                ResourceAvailabilityList::resource_windows.push_back(std::make_shared<model::ResourceWindow>(std::chrono::time_point<std::chrono::system_clock>(std::chrono::milliseconds{0}),
                                                                                                             std::chrono::system_clock::time_point::max(),
                                                                                                             host, i,
                                                                                                             taskResUsage));
            }
    }



    std::pair<int, std::shared_ptr<model::ResourceWindow>>
    ResourceAvailabilityList::containmentQuery(std::chrono::time_point<std::chrono::system_clock> start,
                                               std::chrono::time_point<std::chrono::system_clock> finish) {
        int index = -1;
        std::shared_ptr<model::ResourceWindow> contained_result;
        for (int i = 0; i < ResourceAvailabilityList::resource_windows.size(); i++) {
            auto interval = ResourceAvailabilityList::resource_windows[i];
            std::chrono::time_point<std::chrono::system_clock> interval_start = interval->timeWindow->start;
            std::chrono::time_point<std::chrono::system_clock> interval_finish = interval->timeWindow->stop;

            if(ResourceAvailabilityList::containmentComparison(interval_start, interval_finish, start, finish)) {
                index = i;
                contained_result = interval;
                break;
            }
        }
        return std::make_pair(index, contained_result);
    }

    std::pair<int, std::shared_ptr<model::ResourceWindow>>
    ResourceAvailabilityList::constrainedContainmentQuery(std::chrono::time_point<std::chrono::system_clock> start,
                                                          std::chrono::time_point<std::chrono::system_clock> finish,
                                                          std::string hostname) {
        int index = -1;
        std::shared_ptr<model::ResourceWindow> contained_result;
        for (int i = 0; i < ResourceAvailabilityList::resource_windows.size(); i++) {
            auto interval = ResourceAvailabilityList::resource_windows[i];
            if (hostname != interval->deviceId)
                continue;

            std::chrono::time_point<std::chrono::system_clock> interval_start = interval->timeWindow->start;
            std::chrono::time_point<std::chrono::system_clock> interval_finish = interval->timeWindow->stop;

            if(ResourceAvailabilityList::containmentComparison(interval_start, interval_finish, start, finish)) {
                index = i;
                contained_result = interval;
                break;
            }
        }
        return std::make_pair(index, contained_result);
    }

    bool
    ResourceAvailabilityList::containmentComparison(std::chrono::time_point<std::chrono::system_clock> interval_start,
                                                    std::chrono::time_point<std::chrono::system_clock> interval_finish,
                                                    std::chrono::time_point<std::chrono::system_clock> start,
                                                    std::chrono::time_point<std::chrono::system_clock> finish) {
        if (interval_finish < start)
            return false;
        if (interval_start < start)
            interval_start = start;

        if (interval_start <= start && interval_finish >= finish) return true;

        return false;
    }

    // Function to gather all overlapping tasks
    std::vector<std::pair<int, std::shared_ptr<model::ResourceWindow>>>
    ResourceAvailabilityList::overlapQuery(std::chrono::time_point<std::chrono::system_clock> start,
                 std::chrono::time_point<std::chrono::system_clock> finish) {
        std::vector<std::pair<int, std::shared_ptr<model::ResourceWindow>>> results;

        int selected_track_id = -1;

        for (int i = 0; i < resource_windows.size(); i++) {
            if(resource_windows[i]->track_id == selected_track_id || selected_track_id == -1) {

                auto interval = resource_windows[i];
                auto interval_start = interval->timeWindow->start;
                auto interval_finish = interval->timeWindow->stop;

                if (overlapComparison(interval_start, interval_finish, start, finish)) {
                    results.emplace_back(i, interval);
                    selected_track_id = interval->track_id;
                }
            }
        }

        return results;
    }

    // Function to compare if two time windows overlap
    bool ResourceAvailabilityList::overlapComparison(std::chrono::time_point<std::chrono::system_clock> interval_start,
                                  std::chrono::time_point<std::chrono::system_clock> interval_finish,
                                  std::chrono::time_point<std::chrono::system_clock> window_start,
                                  std::chrono::time_point<std::chrono::system_clock> window_finish) {
        return interval_start < window_finish && interval_finish > window_start;
    }

    void ResourceAvailabilityList::insert(std::vector<std::shared_ptr<model::ResourceWindow>> input_windows){

        int res_list_size = ResourceAvailabilityList::resource_windows.size();
        for(int i = 0; i < res_list_size; i++){
            if(input_windows.front()->timeWindow->stop < ResourceAvailabilityList::resource_windows[i]->timeWindow->stop){
                ResourceAvailabilityList::resource_windows.insert(ResourceAvailabilityList::resource_windows.begin() + i, input_windows.front());
                input_windows.erase(input_windows.begin());
                res_list_size++;

                if (input_windows.empty())
                    break;
            }
        }

        for(const auto& window: input_windows)
            ResourceAvailabilityList::resource_windows.push_back(window);

        utils::verify_res_avail(this);
    }

    void ResourceAvailabilityList::removeItem(int index){
        ResourceAvailabilityList::resource_windows.erase(ResourceAvailabilityList::resource_windows.begin() + index);
//        utils::verify_res_avail(this);
    }

    std::vector<std::pair<int, std::shared_ptr<model::ResourceWindow>>>
    ResourceAvailabilityList::containmentQueryMulti(std::chrono::time_point<std::chrono::system_clock> start,
                                                    std::chrono::time_point<std::chrono::system_clock> finish,
                                                    int task_count) {
        std::vector<std::pair<int, std::shared_ptr<model::ResourceWindow>>> results;
        int t_c = (task_count < ResourceAvailabilityList::device_track_count) ? task_count : ResourceAvailabilityList::device_track_count;

        for (int i = 0; i < ResourceAvailabilityList::resource_windows.size() && t_c > 0; i++) {
            auto interval = ResourceAvailabilityList::resource_windows[i];

            std::chrono::time_point<std::chrono::system_clock> interval_start = interval->timeWindow->start;
            std::chrono::time_point<std::chrono::system_clock> interval_finish = interval->timeWindow->stop;

            if(ResourceAvailabilityList::containmentComparison(interval_start, interval_finish, start, finish)) {
                results.emplace_back(i, interval);
                t_c--;
            }
        }
        return results;
    }

    std::string ResourceAvailabilityList::toString(){
        std::string result = "";

        for(const auto window: ResourceAvailabilityList::resource_windows){
            result += window->toString() + " - ";
        }
        return result;
    };
} // model

