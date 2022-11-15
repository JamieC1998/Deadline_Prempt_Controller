//
// Created by jamiec on 10/4/22.
//

#include "NetworkServices.h"

using namespace std::chrono_literals;

namespace services {
    std::shared_ptr<std::pair<std::chrono::time_point<std::chrono::high_resolution_clock>, std::chrono::time_point<std::chrono::high_resolution_clock>>>
    findLinkSlot(std::chrono::time_point<std::chrono::high_resolution_clock> baseStart, float bW_mbpms,
                 float dataSize, std::shared_ptr<std::vector<std::shared_ptr<model::LinkAct>>> netLink) {
        int index = 0;
        float duration = dataSize / bW_mbpms;
        std::pair<std::chrono::time_point<std::chrono::high_resolution_clock>, std::chrono::time_point<std::chrono::high_resolution_clock>> time_window;
        if (netLink->empty() ||
            netLink->front()->getStartFinTime().first > (baseStart + (1ms * duration)))
            time_window = std::make_pair(baseStart, std::chrono::time_point_cast<std::chrono::milliseconds>(
                    baseStart + (1ms * duration)));
        else {
            for (int i = 0; i < netLink->size(); i++) {
                /* If we are at the end of the time window list */
                if (i == netLink->size() - 1) {
                    index = i + 1;
                    /* And the potential start of our time window is smaller than the last option
                     * in the occupancy list, then we must set the window floor to the largest time
                     * value in the list */
                    if ((*netLink)[i]->getStartFinTime().second > baseStart)
                        time_window = std::make_pair((*netLink)[i]->getStartFinTime().second,
                                                     std::chrono::time_point_cast<std::chrono::milliseconds>(
                                                             (*netLink)[i]->getStartFinTime().second +
                                                             (1ms * duration)));
                        /* Otherwise, if the potential upload start is greater than the largest existing time value */
                    else
                        time_window = std::make_pair(baseStart, std::chrono::time_point_cast<std::chrono::milliseconds>(
                                baseStart + (1ms * duration)));
                    break;
                } else {
                    /* If time window A finished before our upload can start,
                     * then we use our upload start as our potential windows
                     * floor */
                    std::chrono::time_point<std::chrono::high_resolution_clock> floor = ((*netLink)[i]->getStartFinTime().second > baseStart)
                                  ? (*netLink)[i]->getStartFinTime().second : baseStart;
                    if ((*netLink)[i + 1]->getStartFinTime().first - floor >= (1ms * duration)) {
                        index = i + 1;
                        time_window = std::make_pair(floor, std::chrono::time_point_cast<std::chrono::milliseconds>(floor + (1ms * duration)));
                        break;
                    }
                }
            }

        }

        return std::make_shared<std::pair<std::chrono::time_point<std::chrono::high_resolution_clock>, std::chrono::time_point<std::chrono::high_resolution_clock>>>(time_window.first, time_window.second);
    }
} // services