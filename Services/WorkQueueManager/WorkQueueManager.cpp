//
// Created by jamiec on 9/23/22.
//

#include <thread>
#include "WorkQueueManager.h"
#include "../../model/data_models/WorkItems/DAGDisruptItem/DAGDisruption.h"
#include "../../model/data_models/WorkItems/ProcessingItem/ProcessingItem.h"
#include "../../model/data_models/ComputationDevice/ComputationDevice.h"
#include "../LowCompServices/LowCompServices.h"

namespace services {
    std::atomic<int> WorkQueueManager::thread_counter = 0;

    void WorkQueueManager::dag_disruption_call(model::WorkItem *item) {
        WorkQueueManager::thread_counter--;
    }

    void WorkQueueManager::low_comp_allocation_call(model::WorkItem *item) {
        auto* proc_item = reinterpret_cast<model::ProcessingItem*>(item);

        std::unique_lock<std::mutex> net_lock(network_lock);
        std::map<std::string, std::shared_ptr<model::ComputationDevice>> devices = network->getDevices();
        std::shared_ptr<model::ComputationDevice> comp_device = devices[reinterpret_cast<model::WorkItem*>(proc_item)->getHostList()[0]];
        net_lock.unlock();

        std::pair<bool, std::pair<time_t, time_t>> result = services::allocate_task(item, comp_device);
        WorkQueueManager::thread_counter--;
    }

    void WorkQueueManager::high_comp_allocation_call(model::WorkItem *item) {
        WorkQueueManager::thread_counter--;
    }

    void WorkQueueManager::deadline_premp_call(model::WorkItem *item) {
        WorkQueueManager::thread_counter--;
    }

    void WorkQueueManager::halt_call() {
        WorkQueueManager::thread_counter--;
    }

    void WorkQueueManager::add_task(model::WorkItem *item) {
        std::lock_guard<std::mutex> lk(work_queue_lock);

        if (item->getRequestType() == enums::request_type::halt_req) {
            int list_size = static_cast<int>(WorkQueueManager::work_queue.size());
            for (int i = 0; i < list_size; i++) {
                if (WorkQueueManager::work_queue[i]->getRequestType() == enums::request_type::dag_disruption) {
                    work_queue.erase(work_queue.begin() + i);
                    i--;
                    list_size;
                }
            }
        }

        WorkQueueManager::work_queue.push_back(item);

        std::sort(work_queue.begin(), work_queue.end(),
                  [](const model::WorkItem *a, const model::WorkItem *b) {
                      return a->getRequestType() < b->getRequestType();
                  });

    }

    void testFunc(int a, std::string b, double c) {}

    [[noreturn]] void WorkQueueManager::main_loop() {
        std::unique_lock<std::mutex> lk(work_queue_lock, std::defer_lock);
        while (true) {
            WorkQueueManager::current_task.clear();
            WorkQueueManager::thread_counter = 0;

            if (!work_queue.empty()) {
                lk.lock();
                if (work_queue.front()->getRequestType() == enums::request_type::low_complexity) {
                    while (work_queue.front()->getRequestType() == enums::request_type::low_complexity) {
                        WorkQueueManager::thread_counter++;
                        current_task.push_back(work_queue.front());
                        work_queue.erase(work_queue.begin());
                    }
                } else {
                    WorkQueueManager::thread_counter++;
                    current_task.push_back(work_queue.front());
                    work_queue.erase(work_queue.begin());
                }
                lk.unlock();

                std::vector<std::thread> thread_pool;
                switch (current_task.front()->getRequestType()) {
                    case enums::request_type::low_complexity:
                        for (model::WorkItem *item: current_task)
                            thread_pool.emplace_back(low_comp_allocation_call, item);
                        break;
                    case enums::request_type::high_complexity:
                        thread_pool.emplace_back(high_comp_allocation_call, current_task.front());
                        break;
                    case enums::request_type::dag_disruption:
                        thread_pool.emplace_back(dag_disruption_call, current_task.front());
                        break;
                    case enums::request_type::deadline_prempt:
                        thread_pool.emplace_back(deadline_premp_call, current_task.front());
                        break;
                    case enums::request_type::halt_req:
                        thread_pool.emplace_back(halt_call);
                        break;
                }

                while (WorkQueueManager::thread_counter > 0) {
                    lk.lock();
                    if (work_queue.front()->getRequestType() == enums::request_type::low_complexity) {
                        while (work_queue.front()->getRequestType() == enums::request_type::low_complexity) {
                            WorkQueueManager::thread_counter++;
                            thread_pool.emplace_back(low_comp_allocation_call, work_queue.front());
                            current_task.push_back(work_queue.front());
                            work_queue.erase(work_queue.begin());
                        }
                    }
                    lk.unlock();
                }

                for(model::WorkItem* item: WorkQueueManager::work_queue){
                    switch(item->getRequestType()){
                        case enums::request_type::dag_disruption:
                            delete(reinterpret_cast<model::DAGDisruption*> (item));
                            break;
                        case enums::request_type::low_complexity:
                        case enums::request_type::high_complexity:
                            delete(reinterpret_cast<model::ProcessingItem*> (item));
                            break;
                        default:
                            delete(item);
                    }
                }
            }
        }
    }
} // services