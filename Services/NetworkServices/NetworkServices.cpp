//
// Created by jamiec on 10/4/22.
//

#include "NetworkServices.h"

using namespace std::chrono_literals;

namespace services {
    std::shared_ptr<model::TimeWindow>
    findLinkSlot(std::chrono::time_point<std::chrono::system_clock> baseStart, float bw_bpms,
                 uint64_t dataSize, std::vector<std::shared_ptr<model::NetCommunicationBase>> netLink) {
        int index = 0;
        float duration = (static_cast<float>(dataSize) / bw_bpms) + 1;
        auto ms_ceil_duration = static_cast<uint64_t>(std::ceil(duration));
        auto duration_add = std::chrono::milliseconds {ms_ceil_duration};
        std::pair<std::chrono::time_point<std::chrono::system_clock>, std::chrono::time_point<std::chrono::system_clock>> time_window;
        if (netLink.empty() ||
            netLink.front()->timeWindow->start > (baseStart + duration_add))
            time_window = std::make_pair(baseStart, std::chrono::time_point_cast<std::chrono::milliseconds>(
                    baseStart + duration_add));
        else {
            for (int i = 0; i < netLink.size(); i++) {
                /* If we are at the end of the time window list */
                if (i == netLink.size() - 1) {
                    index = i + 1;
                    /* And the potential start of our time window is smaller than the last option
                     * in the occupancy list, then we must set the window floor to the largest time
                     * value in the list */
                    if ((netLink)[i]->timeWindow->stop > baseStart)
                        time_window = std::make_pair((netLink)[i]->timeWindow->stop,
                                                     std::chrono::time_point_cast<std::chrono::milliseconds>(
                                                             (netLink)[i]->timeWindow->stop +
                                                             duration_add));
                        /* Otherwise, if the potential upload start is greater than the largest existing time value */
                    else
                        time_window = std::make_pair(baseStart, std::chrono::time_point_cast<std::chrono::milliseconds>(
                                baseStart + duration_add));
                    break;
                } else {
                    /* If time window A finished before our upload can start,
                     * then we use our upload start as our potential windows
                     * floor */
                    std::chrono::time_point<std::chrono::system_clock> floor = ((netLink)[i]->timeWindow->stop > baseStart)
                                  ? (netLink)[i]->timeWindow->stop : baseStart;
                    if ((netLink)[i + 1]->timeWindow->start - floor >= duration_add) {
                        index = i + 1;
                        time_window = std::make_pair(floor, std::chrono::time_point_cast<std::chrono::milliseconds>(floor + duration_add));
                        break;
                    }
                }
            }

        }

        return std::make_shared<model::TimeWindow>(time_window.first, time_window.second);
    }
} // services