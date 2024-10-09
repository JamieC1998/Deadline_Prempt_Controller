//
// Created by Jamie Cotter on 09/09/2024.
//

#ifndef CONTROLLER_PROCESSINGDATASTRUCTURECONVERSION_H
#define CONTROLLER_PROCESSINGDATASTRUCTURECONVERSION_H

#include <vector>
#include "chrono"
#include <limits>
#include <algorithm>
#include "../../model/data_models/ResourceWindow/ResourceWindow.h"
#include "../../model/data_models/CompResult/BaseCompResult/BaseCompResult.h"

namespace services {
// The parent function that calls everything else
// The last index of the return list represents unbounded time,
// (timestamp of unbounded start, int_max represent unbounded)
    std::vector<std::shared_ptr<model::ResourceWindow>> convertOverlapToResourceAvailability(
            std::vector<std::shared_ptr<model::BaseCompResult>> tasks, int maxUsage,
            std::chrono::time_point<std::chrono::system_clock> currentTimeMs,
            std::string deviceId, std::chrono::milliseconds minimumDuration);

//Input:
//ranges - a list of non overlapping resource usage variations for a given device
//        threshold - the max threshold for resource usage,
//        id - the id of the device
//        minimum_duration - The minimum length of a resource availability window to be considered valid,
//        start_time - the floor for generating the data structure
//
//Given a set of non-overlapping resource usage windows for a device, merge and filter it so that we have a set of windows
//that can accomodate a specified resource capacity.
std::vector<std::shared_ptr<model::ResourceWindow>> generateMergedRangesUnderThreshold(
        std::vector<std::shared_ptr<model::ResourceWindow>> ranges, int threshold,
        std::string id, std::chrono::milliseconds minimumDuration,
        std::chrono::time_point<std::chrono::system_clock> startTime);

std::vector<std::shared_ptr<model::ResourceWindow>> generateSortedListOfCumulativeNonOverlapResourceAvail(
        std::vector<std::shared_ptr<model::ResourceWindow>> resAvails);


//Input:
//sorted_task_list - A sorted list of overlapping tasks for a given device,
//each task contains a start time, finish time and core usage.
//
//Splits each task into a start time which represents resource usage increase
//and finish time which represents resource usage decrease.
//
//Using this data structure it then generates a set of non-overlapping resource usage windows
//for the given device.
//
//Returns:
//A list of non-overlapping resource usage variations, (start time, finish time, resource usage)
    std::vector<std::shared_ptr<model::ResourceWindow>> generateNonOverlappingResourceUsage(
            std::vector<std::shared_ptr<model::BaseCompResult>> sortedTaskList, std::string deviceId);
}

#endif //CONTROLLER_PROCESSINGDATASTRUCTURECONVERSION_H
