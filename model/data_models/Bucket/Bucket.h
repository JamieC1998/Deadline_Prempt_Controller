//
// Created by Jamie Cotter on 09/09/2024.
//

#ifndef CONTROLLER_BUCKET_H
#define CONTROLLER_BUCKET_H

#include <vector>
#include <cpprest/json.h>
#include "../TimeWindow/TimeWindow.h"

namespace model {
    class Bucket {
    public:
        std::shared_ptr<TimeWindow> timeWindow;
        int capacity;

        // Int represents tasks associated
        std::vector<std::string> bucketContents;

        // Constructor
        Bucket(int bucketSize, std::chrono::time_point<std::chrono::system_clock> start, std::chrono::time_point<std::chrono::system_clock> stop);

        web::json::value convertToJson();
    };
}

#endif //CONTROLLER_BUCKET_H
