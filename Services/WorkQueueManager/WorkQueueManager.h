//
// Created by jamiec on 9/23/22.
//

#ifndef CONTROLLER_WORKQUEUEMANAGER_H
#define CONTROLLER_WORKQUEUEMANAGER_H

#include <queue>
#include <cpprest/http_msg.h>
#include "../../model/enums/RequestTypeEnum.h"
#include "../../model/data_models/WorkItems/BaseWorkItem/WorkItem.h"
#include "../../model/data_models/Network/Network.h"
#include "../../model/data_models/CompResult/HighCompResult/HighCompResult.h"
#include "../../model/data_models/WorkItems/BaseWorkItem/WorkItem.h"
#include "../NetworkQueueManager/NetworkQueueManager.h"
#include "../../model/data_models/CompResult/LowCompResult/LowCompResult.h"
#include "../LOG_MANAGER/LogManager.h"
#include "../../Constants/EMA.h"
#include "../../Services/NetworkLinkDiscFunctions/NetworkDiscFunctions.h"
#include "../../Constants/NETWORK_DISC_PARAMS.h"
#include "../../model/data_models/NetworkCommsModels/HighComplexityAllocation/HighComplexityAllocationComms.h"
#include "../ProcessingDataStructureConversion/ProcessingDataStructureConversion.h"
#include "../../Constants/RequestSizes.h"

namespace services {

    class WorkQueueManager {
    public:
        explicit WorkQueueManager(std::shared_ptr<LogManager> ptr, std::shared_ptr<NetworkQueueManager> sharedPtr);

        void add_task(std::shared_ptr<model::WorkItem> item);

        [[noreturn]] static void main_loop(WorkQueueManager *queueManager);

        static void decrementThreadCounter();

        std::mutex work_queue_lock;
        std::mutex network_lock = std::mutex();
        std::mutex offloaded_lock = std::mutex();
        std::shared_ptr<model::Network> network;
        std::shared_ptr<services::NetworkQueueManager> networkQueueManager;

        double getAverageBitsPerSecond() const;

        void setAverageBitsPerSecond(double averageBitsPerSecond);

        double getJitter() const;

        double getBytesPerMillisecond();

        void setJitter(double jitter);

        uint64_t state_size = LOW_TASK_SIZE;

        std::shared_ptr<services::LogManager> logManager;

        bool start = false;
    private:
        static std::atomic<int> thread_counter;
        std::vector<std::shared_ptr<model::WorkItem>> current_task;
        //Maybe needs to be a dequeue
        std::vector<std::shared_ptr<model::WorkItem>> work_queue;
        double average_bits_per_second = 0.0;
        double jitter = 0.0;
    };

    static void state_update_call(std::shared_ptr<model::WorkItem> workItem, WorkQueueManager *queueManager);

    static void low_comp_allocation_call(std::shared_ptr<model::WorkItem> workItem, WorkQueueManager *queueManager);

    static void high_comp_allocation_call(std::shared_ptr<model::WorkItem> workItem, WorkQueueManager *queueManager);

    static void halt_call(std::shared_ptr<model::WorkItem> workItem, WorkQueueManager *queueManager);

    static void update_bw_vals(std::shared_ptr<model::WorkItem> workItem, WorkQueueManager *queueManager);

    static void update_network_disc(std::shared_ptr<model::WorkItem> workItem, WorkQueueManager *queueManager);

    static void regenerate_res_data_structure(std::shared_ptr<model::WorkItem> workItem, WorkQueueManager *queueManager);

    int selectResourceConfig(std::chrono::time_point<std::chrono::system_clock> start,
                             std::chrono::time_point<std::chrono::system_clock> deadline);

    std::vector<int> gatherCommSlots(int task_count, std::vector<std::shared_ptr<model::Bucket>> netlink,
                                     std::chrono::time_point<std::chrono::system_clock> currentTime,
                                     std::chrono::time_point<std::chrono::system_clock> last_time_of_reasoning,
                                     double bytes_per_millisecond
    );

    static void findResourceWindow(std::chrono::time_point<std::chrono::system_clock> start_time,
                                   std::chrono::time_point<std::chrono::system_clock> deadline,
                                   std::shared_ptr<model::ComputationDevice> device, int resourceConfig, int task_count,
                                   std::string host,
                                   std::map<std::string, std::vector<std::pair<int, std::shared_ptr<model::ResourceWindow>>>> &results,
                                   std::mutex &mtx);

    std::pair<std::vector<std::pair<int, std::shared_ptr<model::ResourceWindow>>>, std::vector<std::pair<int, std::shared_ptr<model::ResourceWindow>>>>
    selectResults(
            std::map<std::string, std::vector<std::pair<int, std::shared_ptr<model::ResourceWindow>>>> placement_results,
            std::string sourceHost, int task_count);

    std::tuple<std::vector<int>, std::vector<std::shared_ptr<model::HighCompResult>>, std::vector<std::shared_ptr<model::HighComplexityAllocationComms>>>
    generateHighCompResults(
            std::vector<std::string> dnnIds,
            std::vector<std::pair<int, std::shared_ptr<model::ResourceWindow>>> source_selected_results,
            std::vector<std::pair<int, std::shared_ptr<model::ResourceWindow>>> offloaded_selected_results,
            std::vector<std::shared_ptr<model::Bucket>> network_link,
            std::vector<int> task_comm_windows,
            std::string sourceHost,
            int resourceConfig,
            std::chrono::time_point<std::chrono::system_clock> currentTime,
            uint64_t processing_time,
            int n,
            int m,
            std::chrono::time_point<std::chrono::system_clock> deadline, bool isReallocation
    );

    std::tuple<int, int, uint64_t> selectResourceUsage(int resourceConfig);
} // services

#endif //CONTROLLER_WORKQUEUEMANAGER_H
