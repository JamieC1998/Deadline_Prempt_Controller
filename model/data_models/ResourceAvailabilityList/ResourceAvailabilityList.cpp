//
// Created by Jamie Cotter on 30/09/2024.
//

#include "ResourceAvailabilityList.h"

namespace model {

    ResourceAvailabilityList::ResourceAvailabilityList(int taskResUsage, int deviceTrackCount, int deviceCoreCount,
                                                       std::chrono::milliseconds minProcessingTime,
                                                       std::chrono::time_point<std::chrono::system_clock> start_time,
                                                       std::string hostname) : task_res_usage(
            taskResUsage), device_track_count(deviceTrackCount), device_core_count(deviceCoreCount),
                                                                                             min_processing_time(
                                                                                                     minProcessingTime) {
            for (int i = 0; i < deviceTrackCount; i++) {
                ResourceAvailabilityList::resource_windows.push_back(std::make_shared<model::ResourceWindow>(start_time,
                                                                                                             std::chrono::time_point<std::chrono::system_clock>(
                                                                                                                     std::chrono::milliseconds{
                                                                                                                             INT_MAX}),
                                                                                                             hostname,
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
    }

    void ResourceAvailabilityList::removeItem(int index){
        ResourceAvailabilityList::resource_windows.erase(ResourceAvailabilityList::resource_windows.begin() + index);
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
} // model

