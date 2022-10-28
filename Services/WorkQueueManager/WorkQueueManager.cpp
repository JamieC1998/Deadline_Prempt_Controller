//
// Created by jamiec on 9/23/22.
//

#include <thread>
#include "WorkQueueManager.h"
#include "../../Constants/CompNetCon.h"
#include "../../model/data_models/WorkItems/DAGDisruptItem/DAGDisruption.h"
#include "../../model/data_models/WorkItems/ProcessingItem/ProcessingItem.h"
#include "../LowCompServices/LowCompServices.h"
#include "../../model/data_models/WorkItems/StateUpdate/StateUpdate.h"
#include "../NetworkServices/NetworkServices.h"

namespace services {
    std::atomic<int> WorkQueueManager::thread_counter = 0;

    void WorkQueueManager::dag_disruption_call(model::WorkItem *item) {
        WorkQueueManager::thread_counter--;
    }

    void WorkQueueManager::low_comp_allocation_call(model::WorkItem *item) {
        auto *proc_item = reinterpret_cast<model::ProcessingItem *>(item);

        std::unique_lock<std::mutex> net_lock(network_lock);
        std::map<std::string, std::shared_ptr<model::ComputationDevice>> devices = network->getDevices();
        std::shared_ptr<std::vector<std::shared_ptr<model::LinkAct>>> copyList;
        std::copy(network->getLink().begin(), network->getLink().end(), copyList->begin());
        net_lock.unlock();

        /*TODO IMPLEMENT PROPER BANDWIDTH*/

        float bw = 1;
        float latency = COMM_DELAY_MS;
        /*TODO IMPLEMENT PROPER BANDWIDTH*/

        float data_size_mb = static_cast<float>((reinterpret_cast<model::ProcessingItem *>(item))->getAllocationInputData().at(
                0)->getBaseDnnSize());
        data_size_mb = (data_size_mb / 1024) / 1024;
        std::map<std::string, std::shared_ptr<std::pair<std::chrono::time_point<std::chrono::system_clock>, std::chrono::time_point<std::chrono::system_clock>>>> times;

        for (const auto& name: *item->getHostList()) {
            std::chrono::time_point<std::chrono::system_clock> currentTime = std::chrono::system_clock::now();
            std::shared_ptr<std::pair<std::chrono::time_point<std::chrono::system_clock>, std::chrono::time_point<std::chrono::system_clock>>> time_slot = services::findLinkSlot(
                    currentTime, bw, latency, data_size_mb, copyList);
            times[name] = time_slot;
            copyList->push_back(std::make_shared<model::LinkAct>(*time_slot));

            std::sort(copyList->begin(), copyList->end(),
                      [](const std::shared_ptr<model::LinkAct>& a, const std::shared_ptr<model::LinkAct>& b) {

                          return a->getStartFinTime().second < b->getStartFinTime().second;
                      });
        }

        auto result = services::allocate_task(item, devices, times);

        if (!result.first) {
            auto workItem = new model::WorkItem(enums::request_type::halt_req);
            WorkQueueManager::add_task(workItem);
            auto newTask = new model::ProcessingItem(enums::request_type::low_complexity, item->getHostList(),
                                                     proc_item->getAllocationInputData());
            WorkQueueManager::add_task(reinterpret_cast<model::WorkItem *>(newTask));
        } else {
            for (auto &k_v: *result.second) {
                auto *bR = new model::BaseResult(devices[k_v.first]->getId(), enums::dnn_type::low_comp,
                                                 k_v.first, proc_item->getDeadline().at(k_v.first),
                                                 k_v.second.first, k_v.second.second);

                bR->tasks[0] = std::initializer_list<std::map<int, std::shared_ptr<model::Task>>::value_type>{};

                //TODO UPDATE TILEMAP FROM LOOKUP
                std::shared_ptr<model::Task> task = std::make_shared<model::Task>(bR->getDnnId(),
                                                                                  enums::request_type::low_complexity,
                                                                                  0, -1, 0,
                                                                                  devices[k_v.first]->getId(),
                                                                                  std::make_shared<model::TileRegion>(),
                                                                                  std::make_shared<model::TileRegion>(),
                                                                                  1,
                                                                                  std::initializer_list<std::vector<int>>::value_type{
                                                                                          0},
                                                                                  proc_item->getAllocationInputData().at(
                                                                                          k_v.first)->getRamReq()[0],
                                                                                  proc_item->getAllocationInputData().at(
                                                                                          k_v.first)->getStorageReq()[0],
                                                                                  k_v.second.first,
                                                                                  k_v.second.second,
                                                                                  k_v.first,
                                                                                  std::make_shared<model::LinkAct>(
                                                                                          (true,
                                                                                                  std::make_pair<int, int>(
                                                                                                          -1,
                                                                                                          devices[k_v.first]->getId()),
                                                                                                  std::make_pair<std::string, std::string>(
                                                                                                          std::string(
                                                                                                                  "controller"),
                                                                                                          std::string(
                                                                                                                  k_v.first)),
                                                                                                  data_size_mb,
                                                                                                  (std::chrono::duration_cast<std::chrono::milliseconds>(
                                                                                                          times[k_v.first]->second -
                                                                                                          times[k_v.first]->first)).count(),
                                                                                                  *times[k_v.first])));
                bR->tasks[0][0] = task;


            }
        }

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
                } else if (WorkQueueManager::work_queue[i]->getRequestType() == enums::request_type::halt_req)
                    return;
            }
        } else if (item->getRequestType() == enums::request_type::dag_disruption) {
            int list_size = static_cast<int>(WorkQueueManager::work_queue.size());
            for (int i = 0; i < list_size; i++) {
                if (WorkQueueManager::work_queue[i]->getRequestType() == enums::request_type::halt_req ||
                    WorkQueueManager::work_queue[i]->getRequestType() == enums::request_type::dag_disruption) {
                    return;
                }
            }
        }

        WorkQueueManager::work_queue.push_back(item);

        std::sort(work_queue.begin(), work_queue.end(),
                  [](model::WorkItem *a, model::WorkItem *b) {
                      if (a->getRequestType() == enums::request_type::state_update &&
                          b->getRequestType() == enums::request_type::state_update) {
                          auto a_cast = reinterpret_cast<model::StateUpdate *>(a);
                          auto b_cast = reinterpret_cast<model::StateUpdate *>(b);

                          return a_cast->getTimestamp() < b_cast->getTimestamp();
                      } else
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

                for (model::WorkItem *item: WorkQueueManager::work_queue) {
                    switch (item->getRequestType()) {
                        case enums::request_type::dag_disruption:
                            delete (reinterpret_cast<model::DAGDisruption *> (item));
                            break;
                        case enums::request_type::low_complexity:
                        case enums::request_type::high_complexity:
                            delete (reinterpret_cast<model::ProcessingItem *> (item));
                            break;
                        default:
                            delete (item);
                    }
                }
            }
        }
    }
} // services