//
// Created by Jamie Cotter on 09/09/2024.
//
#include "ProcessingDataStructureConversion.h"
#include "../../model/data_models/ResourceTransformation/ResourceTransformation.h"


namespace services {

    std::vector<std::shared_ptr<model::ResourceWindow>> convertOverlapToResourceAvailability(
            std::vector<std::shared_ptr<model::BaseCompResult>> tasks, int maxUsage,
            std::chrono::time_point<std::chrono::system_clock> currentTimeMs,
            std::string deviceId, std::chrono::milliseconds minimumDuration) {

        auto nonOverlappingRanges = generateNonOverlappingResourceUsage(tasks, deviceId);

        if (nonOverlappingRanges.front()->timeWindow->start > currentTimeMs && (nonOverlappingRanges.front()->timeWindow->start - currentTimeMs) > minimumDuration) {
            nonOverlappingRanges.insert(nonOverlappingRanges.begin(),
                                        std::make_shared<model::ResourceWindow>(currentTimeMs, nonOverlappingRanges.front()->timeWindow->start, 0));
        }

//        NEED TO INSERT FOR EACH DEVICE?
        nonOverlappingRanges.push_back(
                std::make_shared<model::ResourceWindow>(nonOverlappingRanges.back()->timeWindow->stop, std::chrono::time_point<std::chrono::system_clock>(std::chrono::milliseconds(std::numeric_limits<int>::max())), 0));

        return generateMergedRangesUnderThreshold(nonOverlappingRanges, maxUsage, deviceId, minimumDuration,
                                                  currentTimeMs);
    }

    std::vector<std::shared_ptr<model::ResourceWindow>> generateMergedRangesUnderThreshold(
            std::vector<std::shared_ptr<model::ResourceWindow>> ranges, int threshold,
            std::string id, std::chrono::milliseconds minimumDuration,
            std::chrono::time_point<std::chrono::system_clock> startTime) {

        std::vector<std::shared_ptr<model::ResourceWindow>> mergedRanges;
        std::chrono::time_point<std::chrono::system_clock> currentStart  = ranges.front()->timeWindow->start;
        std::chrono::time_point<std::chrono::system_clock> currentEnd = ranges.front()->timeWindow->stop;

        auto currentCapacity = ranges.front()->capacity;

        for (size_t i = 1; i < ranges.size(); ++i) {
            std::chrono::time_point<std::chrono::system_clock> start = ranges[i]->timeWindow->start;
            std::chrono::time_point<std::chrono::system_clock> end = ranges[i]->timeWindow->stop;
            auto capacity = ranges[i]->capacity;

            if (capacity <= threshold) {
                currentEnd = end;
                if (capacity > currentCapacity) {
                    currentCapacity = capacity;
                }
            } else {
                if ((currentEnd - currentStart) >= minimumDuration && currentEnd >= startTime) {
                    mergedRanges.push_back(std::make_shared<model::ResourceWindow>(std::max(currentStart, startTime), currentEnd, id, 0,currentCapacity));
                }
                currentStart = end;
                currentCapacity = 0;
            }
        }

        mergedRanges.push_back(std::make_shared<model::ResourceWindow>(currentStart, currentEnd, id, 0, currentCapacity));

        return mergedRanges;
    }

    std::vector<std::shared_ptr<model::ResourceWindow>> generateSortedListOfCumulativeNonOverlapResourceAvail(
            std::vector<std::shared_ptr<model::ResourceWindow>> resAvails) {

        std::vector<std::shared_ptr<model::ResourceWindow>> result;

        for (auto subRes: resAvails)
            result.push_back(subRes);

        std::sort(result.begin(), result.end(), [](std::shared_ptr<model::ResourceWindow> a, std::shared_ptr<model::ResourceWindow> b) {
            return a->timeWindow->start < b->timeWindow->start;
        });

        return result;
    }

    std::vector<std::shared_ptr<model::ResourceWindow>> generateNonOverlappingResourceUsage(
            std::vector<std::shared_ptr<model::BaseCompResult>> sortedTaskList, std::string deviceId) {

        std::vector<model::ResourceTransformation> resourceVariance;

        for (const auto& task: sortedTaskList) {
            resourceVariance.emplace_back(true, task->estimated_start_fin->start, task->getCoreAllocation());
            resourceVariance.emplace_back(false, task->estimated_start_fin->stop, task->getCoreAllocation());
        }

        std::sort(resourceVariance.begin(), resourceVariance.end(),
                  [](const model::ResourceTransformation &a, const model::ResourceTransformation &b) {
                      return a.timestamp < b.timestamp;
                  });

        std::vector<std::shared_ptr<model::ResourceWindow>> nonOverlappingUsage;
        auto currentStart = resourceVariance.front().timestamp;
        auto currentCapacity = resourceVariance.front().resourceUsage;

        for (auto i = 1; i < resourceVariance.size(); ++i) {
            nonOverlappingUsage.emplace_back(std::make_shared<model::ResourceWindow>(currentStart, resourceVariance[i].timestamp, deviceId, 0, currentCapacity));
            currentStart = resourceVariance[i].timestamp;

            if (resourceVariance[i].isIncrease) {
                currentCapacity += resourceVariance[i].resourceUsage;
            } else {
                currentCapacity -= resourceVariance[i].resourceUsage;
            }
        }

        return nonOverlappingUsage;
    }

}