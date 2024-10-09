#include "Bucket.h"

namespace model {
// Constructor Implementation
    Bucket::Bucket(int bucketSize, std::chrono::time_point<std::chrono::system_clock> start,
                   std::chrono::time_point<std::chrono::system_clock> stop)
            : capacity(bucketSize), timeWindow(std::make_shared<TimeWindow>(start, stop)) {
        // Initialize bucket contents as empty
        bucketContents = std::vector<std::string>();
    }

    web::json::value Bucket::convertToJson() {
        web::json::value result;

        web::json::value device_ids = web::json::value::array();

        for(int i = 0; i < Bucket::bucketContents.size(); i++){
            device_ids[i] = web::json::value::string(Bucket::bucketContents[i]);
        }

        result["bucket_contents"] = device_ids;
        result["time_window"] = Bucket::timeWindow->convertToJson();
        result["capacity"] = web::json::value::number(Bucket::capacity);

        return result;

    }
}