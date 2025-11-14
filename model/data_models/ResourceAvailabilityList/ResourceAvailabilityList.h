//
// Created by Jamie Cotter on 30/09/2024.
//

#ifndef CONTROLLER_RESOURCEAVAILABILITYLIST_H
#define CONTROLLER_RESOURCEAVAILABILITYLIST_H

#include "vector"
#include "../ResourceWindow/ResourceWindow.h"

namespace model {

    class ResourceAvailabilityList {
    public:
        std::vector<std::shared_ptr<model::ResourceWindow>> resource_windows = {};

        int task_res_usage = 0;
        int device_track_count = 0;
        int device_core_count = 0;
        std::chrono::milliseconds min_processing_time;

        std::pair<int, std::shared_ptr<model::ResourceWindow>>
        containmentQuery(std::chrono::time_point<std::chrono::system_clock> start,
                         std::chrono::time_point<std::chrono::system_clock> finish);

        std::pair<int, std::shared_ptr<model::ResourceWindow>>
        constrainedContainmentQuery(std::chrono::time_point<std::chrono::system_clock> start,
                         std::chrono::time_point<std::chrono::system_clock> finish, std::string hostname);

        bool overlapComparison(std::chrono::time_point<std::chrono::system_clock> interval_start,
                               std::chrono::time_point<std::chrono::system_clock> interval_finish,
                               std::chrono::time_point<std::chrono::system_clock> window_start,
                               std::chrono::time_point<std::chrono::system_clock> window_finish);

        std::vector<std::pair<int, std::shared_ptr<model::ResourceWindow>>>
        overlapQuery(std::chrono::time_point<std::chrono::system_clock> start,
                                               std::chrono::time_point<std::chrono::system_clock> finish);

        static bool
        containmentComparison(std::chrono::time_point<std::chrono::system_clock> interval_start,
                                                        std::chrono::time_point<std::chrono::system_clock> interval_finish,
                                                        std::chrono::time_point<std::chrono::system_clock> start,
                                                        std::chrono::time_point<std::chrono::system_clock> finish);

        std::vector<std::pair<int, std::shared_ptr<model::ResourceWindow>>>
        containmentQueryMulti(std::chrono::time_point<std::chrono::system_clock> start,
                         std::chrono::time_point<std::chrono::system_clock> finish, int task_count);

        ResourceAvailabilityList(int taskResUsage, int deviceTrackCount, int deviceCoreCount,
                                 std::chrono::milliseconds minProcessingTime,
                                 std::chrono::time_point<std::chrono::system_clock> start_time,
                                 std::string hostname);

        std::string toString();

        void insert(std::vector<std::shared_ptr<model::ResourceWindow>> input_windows);

        void removeItem(int index);
    };

} // model

#endif //CONTROLLER_RESOURCEAVAILABILITYLIST_H
