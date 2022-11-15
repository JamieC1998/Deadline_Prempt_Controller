//
// Created by jamiec on 9/23/22.
//

#include <thread>
#include "WorkQueueManager.h"
#include "../../model/data_models/WorkItems/DAGDisruptItem/DAGDisruption.h"
#include "../../model/data_models/WorkItems/ProcessingItem/ProcessingItem.h"
#include "../LowCompServices/LowCompServices.h"
#include "../../model/data_models/WorkItems/StateUpdate/StateUpdate.h"
#include "../NetworkServices/NetworkServices.h"
#include "../../utils/UtilFunctions/UtilFunctions.h"
#include "../../Constants/CLIENT_DETAILS.h"
#include "../HighCompServices/HighCompServices.h"

namespace services {
    std::atomic<int> WorkQueueManager::thread_counter = 0;

    void dag_disruption_call(model::WorkItem *item, WorkQueueManager *queueManager) {

        services::WorkQueueManager::decrementThreadCounter();
    }

    /*Function receives a list of low complexity DNN allocation requests*/
    void low_comp_allocation_call(model::WorkItem *item, WorkQueueManager *queueManager) {
        auto *proc_item = reinterpret_cast<model::ProcessingItem *>(item);

        std::unique_lock<std::mutex> net_lock(queueManager->network_lock);
        std::map<std::string, std::shared_ptr<model::ComputationDevice>> devices = queueManager->network->getDevices();

        /*Need to create a copy of the network list so that we can keep track of incomplete net allocations */
        std::shared_ptr<std::vector<std::shared_ptr<model::LinkAct>>> copyList;
        std::copy(queueManager->network->getLink().begin(), queueManager->network->getLink().end(), copyList->begin());
        net_lock.unlock();


        /* Fetching bytes and latency from the stored net parameters from iperf test */
        double bw_bytes = (queueManager->getAverageBitsPerSecond() / 8);
        double latency_bytes = queueManager->getJitter() / 8;
        bw_bytes -= latency_bytes;

        float data_size_bytes = static_cast<float>((reinterpret_cast<model::ProcessingItem *>(item))->getAllocationInputData().at(
                0)->getBaseDnnSize());
        /* -------------------------------------------------------------------------- */

        std::map<std::string, std::shared_ptr<std::pair<std::chrono::time_point<std::chrono::high_resolution_clock>, std::chrono::time_point<std::chrono::high_resolution_clock>>>> times;

        /* Need to find the earliest window for transferring data to each host */
        for (const auto &name: *item->getHostList()) {
            std::chrono::time_point<std::chrono::high_resolution_clock> currentTime = std::chrono::high_resolution_clock::now();
            std::shared_ptr<std::pair<std::chrono::time_point<std::chrono::high_resolution_clock>, std::chrono::time_point<std::chrono::high_resolution_clock>>> time_slot = services::findLinkSlot(
                    currentTime, bw_bytes, data_size_bytes, copyList);
            times[name] = time_slot;
            copyList->push_back(std::make_shared<model::LinkAct>(*time_slot));

            std::sort(copyList->begin(), copyList->end(),
                      [](const std::shared_ptr<model::LinkAct> &a, const std::shared_ptr<model::LinkAct> &b) {

                          return a->getStartFinTime().second < b->getStartFinTime().second;
                      });
        }

        /* Using the input upload window for each task generated we now allocate a time slot on their respective hosts */
        auto result = services::allocate_task(item, devices, times);

        /* If we cannot allocate a device for even one task we instead create a new allocation request and a halt request */
        //TODO double check if I'm clearing low complexity with high comp, if so, bad */
        if (!result.first) {
            auto workItem = new model::WorkItem(enums::request_type::halt_req);
            queueManager->add_task(workItem);
            auto newTask = new model::ProcessingItem(enums::request_type::low_complexity, item->getHostList(),
                                                     proc_item->getAllocationInputData());
            queueManager->add_task(reinterpret_cast<model::WorkItem *>(newTask));
            services::WorkQueueManager::decrementThreadCounter();
            return;

        } else {
            /* For each allocated low comp task we create a Result object */
            std::map<std::string, std::shared_ptr<model::BaseResult>> bRList;
            for (auto &k_v: *result.second) {
                auto bR = std::make_shared<model::BaseResult>(
                        model::BaseResult(devices[k_v.first]->getId(), enums::dnn_type::low_comp,
                                          k_v.first, proc_item->getDeadline().at(k_v.first),
                                          k_v.second.first, k_v.second.second));

                bR->tasks[0] = std::initializer_list<std::map<int, std::shared_ptr<model::Task>>::value_type>{};

                //TODO UPDATE TILEMAP FROM LOOKUP
                std::shared_ptr<model::Task> task = std::make_shared<model::Task>(bR->getDnnId(),
                                                                                  enums::dnn_type::low_comp,
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
                                                                                                  data_size_bytes,
                                                                                                  (std::chrono::duration_cast<std::chrono::milliseconds>(
                                                                                                          times[k_v.first]->second -
                                                                                                          times[k_v.first]->first)).count(),
                                                                                                  *times[k_v.first])));
                bR->tasks[0][0] = task;

                std::unique_lock<std::mutex> uniqueLock(queueManager->offloaded_lock);
                std::map<std::string, std::shared_ptr<model::BaseResult>> &low_map = queueManager->getOffLow();
                low_map[std::to_string(bR->getDnnId())] = bR;
                std::map<std::string, std::shared_ptr<model::BaseResult>> &total_map = queueManager->getOffTotal();
                total_map[std::to_string(bR->getDnnId())] = bR;
                uniqueLock.unlock();

                bRList[k_v.first] = bR;
            }

            /* Add communication times to the network link */
            net_lock.lock();
            std::vector<std::shared_ptr<model::LinkAct>> linkActs;
            for (auto [k, v]: times) {
                bool isMeta = false;
                std::pair<int, int> deviceIds = std::make_pair(-1, devices[k]->getId());
                std::pair<std::string, std::string> hostNames = std::make_pair("controller", k);
                float dataSize = data_size_bytes;
                std::pair<std::chrono::time_point<std::chrono::high_resolution_clock>, std::chrono::time_point<std::chrono::high_resolution_clock>> duration_window = std::make_pair(
                        v->first, v->second);
                std::shared_ptr<model::LinkAct> linkAct = std::make_shared<model::LinkAct>(isMeta, deviceIds, hostNames,
                                                                                           dataSize, duration_window);
                linkActs.push_back(linkAct);
            }
            queueManager->network->addComms(linkActs);
            queueManager->network->sortLink();

            for (auto [k, v]: bRList) {
                //TODO ADD FILE PATH
                std::shared_ptr<model::BaseNetworkCommsModel> baseNetworkCommsModel = std::make_shared<model::BaseNetworkCommsModel>(
                        std::initializer_list<std::string>{k}, enums::network_comms_types::task_mapping,
                        times[k]->first, v, "");
                queueManager->networkQueueManager->addTask(baseNetworkCommsModel);
            }
        }

        services::WorkQueueManager::decrementThreadCounter();
    }

    void high_comp_allocation_call(model::WorkItem *item, WorkQueueManager *queueManager) {
        auto *processingItem = reinterpret_cast<model::ProcessingItem *>(item);

        //Fetching Base DNN Topology
        std::string sourceHost = (*item->getHostList())[0];
        std::shared_ptr<model::BaseDNNModel> baseDnnModel = processingItem->getItem(sourceHost);

        std::vector<std::vector<int>> fused_layers;
        std::vector<int> temp_list;

        //Gathering base data comm size
        double bw_bytes = (queueManager->getAverageBitsPerSecond() / 8);
        double latency_bytes = queueManager->getJitter() / 8;
        bw_bytes -= latency_bytes;

        float state_data_size_bytes = static_cast<float>((reinterpret_cast<model::ProcessingItem *>(item))->getAllocationInputData().at(
                0)->getBaseDnnSize());

        int image_size = utils::filesize(processingItem->getInputPath().at(sourceHost));
//        float initial_data_size = static_cast<float>(image_size) + state_data_size_bytes;

        //Need to fuse blocks;
        for (int i = 0; i < baseDnnModel->getLayerType().size(); i++) {
            if (i + 1 >= baseDnnModel->getLayerType().size() ||
                baseDnnModel->getLayerType()[i + 1] == enums::LayerTypeEnum::pooling) {
                temp_list.push_back(i);
                fused_layers.emplace_back();
                std::copy(temp_list.begin(), temp_list.end(), fused_layers.back().begin());
                temp_list.clear();
            } else {
                temp_list.push_back(i);
            }
        }

        std::chrono::time_point<std::chrono::high_resolution_clock> currentTime = std::chrono::high_resolution_clock::now();
        //Need a copy of the network link so we dont mutate it in event of failed allocation
        std::vector<std::shared_ptr<model::LinkAct>> copyLink;
        std::copy(queueManager->network->getLink().begin(), queueManager->network->getLink().end(), copyLink.begin());

        //Will hold the actual link slots so we copy in the entire copied network link;
        std::vector<std::shared_ptr<model::LinkAct>> linkSlots;

        std::map<int, std::map<int, std::shared_ptr<model::Task>>> task_map;

        std::map<int, std::shared_ptr<model::Task>> result_block;
        int netCoreBaseLine = MAX_CORES / 2;

        //Need a temporary task list so we dont pollute the network with failed dnns tasks
        std::map<int, std::vector<std::shared_ptr<model::Task>>> temp_device_task_list;
        for (auto [device_id, device]: queueManager->network->getDevices()) {
            std::vector<std::shared_ptr<model::Task>> tmp_task;
            std::copy(device->getTasks().begin(), device->getTasks().end(), tmp_task.begin());
            temp_device_task_list[device->getId()] = tmp_task;
        }

        int j = 0;
        for (auto fused_block: fused_layers) {
            enums::LayerTypeEnum layerType = baseDnnModel->getLayerType()[fused_block[0]];
            if(layerType == enums::LayerTypeEnum::conv){
                bool allocation_found = false;
                for (int current_core = netCoreBaseLine; current_core >= 1; current_core--) {
                    std::map<int, std::shared_ptr<model::Task>> temp_block;

                    float initial_data_size = (static_cast<float>(image_size) / (static_cast<float>(current_core)) +
                                               state_data_size_bytes);

                    //Need copy of task list so we dont ruin temp task list
                    std::map<int, std::vector<std::shared_ptr<model::Task>>> loop_task_list;

                    for (auto [device_id, device]: loop_task_list) {
                        std::vector<std::shared_ptr<model::Task>> tmp_task;
                        std::copy(device.begin(), device.end(), tmp_task.begin());
                        loop_task_list[device_id] = tmp_task;
                    }

                    //Need a copy of the network link so we dont mutate it in event of failed allocation
                    std::shared_ptr<std::vector<std::shared_ptr<model::LinkAct>>> tempLink;
                    std::copy(queueManager->network->getLink().begin(), queueManager->network->getLink().end(),
                              tempLink->begin());

                    //Will hold the actual link slots so we copy in the entire copied network link;
                    std::vector<std::shared_ptr<model::LinkAct>> loopLinkSlots;
                    //Iterate through each of the cores to find a valid allocation
                    for (int i = 0; i < current_core; i++) {
                        auto time_slot = services::findLinkSlot(
                                (!j) ? currentTime : task_map[j - 1][0]->getEstimatedFinish(), bw_bytes, initial_data_size, tempLink);

                        std::shared_ptr<model::LinkAct> linkActivity;
                        linkActivity->setIsMeta(false);
                        linkActivity->setDataSize(initial_data_size);
                        linkActivity->setStartFinTime(std::make_pair(time_slot->first, time_slot->second));

                        //TODO FETCH ACCURATE PROCESSING TIME FROM FTP LOOKUP TABLE
                        std::chrono::time_point<std::chrono::high_resolution_clock> finish_time;
                        auto selected_device = services::findNode(queueManager->network->getDevices(), loop_task_list,
                                                                  time_slot->second, finish_time);

                        std::shared_ptr<std::pair<std::chrono::time_point<std::chrono::high_resolution_clock>, std::chrono::time_point<std::chrono::high_resolution_clock>>> state_update_time_slot = services::findLinkSlot(
                                finish_time, bw_bytes, utils::calculateStateUpdateSize(), tempLink);

                        if (selected_device.first != -1) {
                            std::shared_ptr<model::Task> t1 = std::make_shared<model::Task>();
                            t1->setEstimatedStart(time_slot->second);
                            t1->setEstimatedFinish(finish_time);
                            t1->setBlockParentId(j - 1);
                            t1->setGroupBlockId(j);
                            t1->setPartitionModelId(i);
                            t1->setAllocatedDeviceId(selected_device.first);
                            t1->setAllocatedHost(selected_device.second);
                            loop_task_list[selected_device.first].push_back(t1);
                            temp_block[i] = t1;

                            linkActivity->setDevIds(std::make_pair((!j) ? -1 : task_map[j - 1][0]->getAllocatedDeviceId(), selected_device.first));
                            linkActivity->setHostNames(std::make_pair((!j) ? "controller" : task_map[j - 1][0]->getAllocatedHost(), selected_device.second));
                            tempLink->push_back(linkActivity);
                            loopLinkSlots.push_back(linkActivity);

                            std::shared_ptr<model::LinkAct> linkAct1;
                            linkAct1->setIsMeta(true);
                            linkAct1->setDevIds(std::make_pair(selected_device.first, -1));
                            linkAct1->setHostNames(std::make_pair(selected_device.second, "controller"));
                            linkAct1->setDataSize(utils::calculateStateUpdateSize());
                            linkAct1->setStartFinTime(std::make_pair(state_update_time_slot->first, state_update_time_slot->second));
                            tempLink->push_back(linkAct1);
                            loopLinkSlots.push_back(linkAct1);
                        }
                        //If one of the partitions cant be allocated in this core config, break
                        else
                            break;
                    }

                    //If we have allocated a device to every partition
                    if (temp_block.size() == current_core) {
                        task_map[j] = temp_block;
                        temp_device_task_list = loop_task_list;
                        linkSlots = loopLinkSlots;

                        copyLink.clear();
                        std::copy(copyLink.begin(), copyLink.end(), tempLink->begin());
                        allocation_found = true;
                        break;
                    }
                }

                if (!allocation_found) {

                    //TODO EXIT
                }
            }
            else if(layerType != enums::LayerTypeEnum::conv){
                std::vector<std::shared_ptr<model::LinkAct>> upload_slots;
                std::chrono::time_point<std::chrono::high_resolution_clock> max_upload_time = task_map[j - 1][0]->getEstimatedFinish();

                std::vector<std::pair<int, std::chrono::time_point<std::chrono::high_resolution_clock>>> source_locations_and_finish_times;
                for(auto task: task_map[j - 1]){
                    max_upload_time = max(task.second->getEstimatedFinish(), max_upload_time);
                    source_locations_and_finish_times.emplace_back(task.second->getAllocatedDeviceId(), task.second->getEstimatedFinish());
                }
                auto time_slot = services::findLinkSlot(
                        currentTime, bw_bytes, initial_data_size, tempLink);

                std::shared_ptr<model::LinkAct> linkAct;
                linkAct->setIsMeta(false);
                linkAct->setDataSize(initial_data_size);
                linkAct->setStartFinTime(std::make_pair(time_slot->first, time_slot->second));

                //TODO FETCH ACCURATE PROCESSING TIME FROM FTP LOOKUP TABLE
                std::chrono::time_point<std::chrono::high_resolution_clock> finish_time;
                auto selected_device = services::findNode(queueManager->network->getDevices(), loop_task_list,
                                                          time_slot->second, finish_time);

                std::shared_ptr<std::pair<std::chrono::time_point<std::chrono::high_resolution_clock>, std::chrono::time_point<std::chrono::high_resolution_clock>>> state_update_time_slot = services::findLinkSlot(
                        finish_time, bw_bytes, utils::calculateStateUpdateSize(), tempLink);

                if (selected_device.first != -1) {
                    std::shared_ptr<model::Task> t1 = std::make_shared<model::Task>();
                    t1->setEstimatedStart(time_slot->second);
                    t1->setEstimatedFinish(finish_time);
                    t1->setBlockParentId(-1);
                    t1->setGroupBlockId(0);
                    t1->setPartitionModelId(i);
                    t1->setAllocatedDeviceId(selected_device.first);
                    t1->setAllocatedHost(selected_device.second);
                    loop_task_list[selected_device.first].push_back(t1);
                    temp_block[i] = t1;

                    std::shared_ptr<model::LinkAct> linkAct;
                    linkAct->setIsMeta(false);
                    linkAct->setDevIds(std::make_pair(-1, selected_device.first));
                    linkAct->setHostNames(std::make_pair("controller", selected_device.second));
                    linkAct->setDataSize(initial_data_size);
                    linkAct->setStartFinTime(std::make_pair(time_slot->first, time_slot->second));
                    tempLink->push_back(linkAct);
                    loopLinkSlots.push_back(linkAct);

                    std::shared_ptr<model::LinkAct> linkAct1;
                    linkAct->setIsMeta(true);
                    linkAct->setDevIds(std::make_pair(selected_device.first, -1));
                    linkAct->setHostNames(std::make_pair(selected_device.second, "controller"));
                    linkAct->setDataSize(utils::calculateStateUpdateSize());
                    linkAct->setStartFinTime(std::make_pair(state_update_time_slot->first, state_update_time_slot->second));
                    tempLink->push_back(linkAct1);
                    loopLinkSlots.push_back(linkAct1);
                }
            }
            j++;
        }

        services::WorkQueueManager::decrementThreadCounter();
    }

    void deadline_premp_call(model::WorkItem *item, WorkQueueManager *queueManager) {
        auto *dag_item = reinterpret_cast<model::DAGDisruption *>(item);

        //List that will hold impacted DNNs
        std::vector<std::shared_ptr<model::BaseResult>> impacted_dnn_list;

        //{parition_model_id: The unique DNN ID, group_block_id: the block group the partition belongs to, partitioned_dnn_id: the id assigned the partitioned block}
        int partition_model_id = dag_item->getPartitionId();
        int block_partition_id = dag_item->getBlockId();
        int partitioned_dnn_id = dag_item->getPartitionedDnnId();
        int unqiue_task_id = queueManager->getOffTotal()[std::to_string(
                partitioned_dnn_id)]->tasks[block_partition_id][partitioned_dnn_id]->getUniqueTaskId();

        //Push back first violated DNN into violated list
        impacted_dnn_list.push_back(queueManager->getOffTotal()[std::to_string(partitioned_dnn_id)]);

        //If the deadline has already been violated exit early
        if (impacted_dnn_list[0]->getEstimatedFinish() + (dag_item->getFinishTime() - dag_item->getEstimatedFinish()) >
            impacted_dnn_list[0]->getDeadline()) {
            auto workItem = new model::WorkItem(enums::request_type::halt_req);
            queueManager->add_task(workItem);
            services::WorkQueueManager::decrementThreadCounter();

            return;
        }

        //Keeping track of the currently affected partitions and pushing in our violated partition task
//        std::vector<std::vector<int>> affected_group_block_index = std::initializer_list<std::vector<int>>{std::initializer_list<int>{0, partitioned_dnn_id, block_partition_id, partition_model_id}};

        std::vector<std::map<std::string, int>> affected_group_block_index = std::initializer_list<std::map<std::string, int>>{
                {
                        {"index", 0},
                        {"dnn_id", partitioned_dnn_id},
                        {"group_block_id", block_partition_id},
                        {"block_id", partition_model_id},
                        {"task_id", unqiue_task_id},
                }
        };

        std::map<int, std::pair<std::chrono::time_point<std::chrono::high_resolution_clock>, std::chrono::time_point<std::chrono::high_resolution_clock>>> violation_map{
                {unqiue_task_id, std::make_pair(dag_item->getFinishTime(), dag_item->getEstimatedFinish())}
        };
        while (!affected_group_block_index.empty()) {
            std::vector<std::map<std::string, int>> temp_affected_group_block_index;

            //We iterate through each victim to see if they impact any subsequent tasks on their device
            for (auto block: affected_group_block_index) {
                std::vector<std::map<std::string, int>> temp_list;

                //Fetching the current victim task
                std::shared_ptr<model::Task> mp = queueManager->getOffTotal()[std::to_string(
                        block["dnn_id"])]->tasks[block["group_block_id"]][block["block_id"]];

                std::string allocated_host = mp->getAllocatedHost();

                std::shared_ptr<model::ComputationDevice> device = queueManager->network->getDevices()[allocated_host];

                //Going through the device the victim task was assigned to and seeing if they impact any subsequent tasks
                for (auto task: device->getTasks()) {

                    //TODO ACCURATE VIOLATION CALC
                    if (task->getRequestType() == enums::dnn_type::high_comp &&
                        task->getEstimatedStart() >= violation_map[block["task_id"]].second &&
                        task->getEstimatedStart() <=
                        violation_map[block["task_id"]].first) {
                        //If the task is violated, add it to the temp list
                        temp_list.emplace_back(std::initializer_list<std::pair<const std::string, int>>{{"index",          -1},
                                                                                                        {"dnn_id",         task->getDnnId()},
                                                                                                        {"group_block_id", task->getGroupBlockId()},
                                                                                                        {"block_id",       task->getPartitionModelId()},
                                                                                                        {"task_id",        task->getUniqueTaskId()}
                        });

                        violation_map[task->getUniqueTaskId()] = std::make_pair(
                                (violation_map[block["task_id"]].first - task->getEstimatedStart()) +
                                task->getEstimatedFinish(), task->getEstimatedFinish());
                    }
                }

                //Checking to see if any of the gathered victims are part of the same DNN as the source
                bool same_dnn = false;
                for (auto temp: temp_list) {
                    if (temp["dnn_id"] == block["dnn_id"])
                        same_dnn = true;
                }

                //If it's not the same DNN we have to add the impacted DNN to the list
                if (!same_dnn) {
                    bool dnn_already_impacted = true;

                    int counter = 0;
                    std::map<std::string, int> selected_victim;

                    //Iterate through the list until we've exhausted all potential victims
                    while (dnn_already_impacted && counter < temp_list.size()) {
                        dnn_already_impacted = false;
                        //Retrieve our selected victim
                        selected_victim = temp_list[counter];

                        //Iterate through our existing impacted DNNs and check if the
                        for (const auto &dnn: impacted_dnn_list) {
                            if (dnn->getDnnId() == selected_victim["dnn_id"]) {
                                dnn_already_impacted = true;
                                break;
                            }
                        }

                        //If the DNN hasn't already been impacted we need to update its start and finish time
                        if (!dnn_already_impacted) {
                            //Find the dnn for the selected victim
                            std::string dnn_id = std::to_string(
                                    selected_victim["dnn_id"]);
                            std::shared_ptr<model::Task> task = queueManager->getOffTotal()[dnn_id]->tasks[selected_victim["group_block_id"]][selected_victim["block_id"]];
                            auto violation = (violation_map[task->getUniqueTaskId()].first -
                                              violation_map[task->getUniqueTaskId()].second);

                            std::shared_ptr<model::BaseResult> part = queueManager->getOffTotal()[dnn_id];

                            selected_victim["index"] = static_cast<int>(impacted_dnn_list.size());

                            if (queueManager->getOffTotal()[dnn_id]->getDeadline() <
                                queueManager->getOffTotal()[dnn_id]->getEstimatedFinish() + violation) {
                                auto workItem = new model::WorkItem(enums::request_type::halt_req);
                                queueManager->add_task(workItem);
                                services::WorkQueueManager::decrementThreadCounter();
                            } else {
                                impacted_dnn_list.push_back(part);
                                temp_affected_group_block_index.push_back(selected_victim);

                                task->setEstimatedStart(task->getEstimatedStart() + violation);
                                task->setEstimatedFinish(task->getEstimatedFinish() + violation);
                                queueManager->getOffTotal()[dnn_id]->setEstimatedFinish(
                                        queueManager->getOffTotal()[dnn_id]->getEstimatedFinish() + violation);
                            }
                        }
                        counter++;
                    }
                }

                std::shared_ptr<model::BaseResult> new_mod;

                for (const auto &pt: queueManager->getOffTotal()) {
                    if (block["dnn_id"] == pt.second->getDnnId()) {
                        new_mod = pt.second;
                        break;
                    }
                }

                if (block["group_block_id"] + 1 == new_mod->tasks.size())
                    continue;

                for (int i = 0; i < new_mod->tasks.size(); i++) {

                    std::map<std::string, int> temp = {
                            {"index",          block["index"]},
                            {"dnn_id",         new_mod->getDnnId()},
                            {"group_block_id", block["group_block_id"] + 1},
                            {"block_id",       i},
                            {"task_id",        new_mod->tasks[block["group_block_id"] + 1][i]->getUniqueTaskId()}
                    };
                    temp_affected_group_block_index.push_back(temp);
                    std::shared_ptr<model::Task> tmp_task = impacted_dnn_list[temp["dnn_id"]]->tasks[temp["group_block_id"]][temp["block_id"]];

                    auto violation_val = (violation_map[block["block_id"]].first - tmp_task->getEstimatedStart());
                    tmp_task->setEstimatedStart(tmp_task->getEstimatedStart() + violation_val);
                    violation_map[tmp_task->getUniqueTaskId()] = std::make_pair(
                            tmp_task->getEstimatedFinish() + violation_val, tmp_task->getEstimatedFinish());
                    tmp_task->setEstimatedFinish(tmp_task->getEstimatedFinish() + violation_val);

                }

                affected_group_block_index = temp_affected_group_block_index;
            }
        }

        services::WorkQueueManager::decrementThreadCounter();
    }

    void halt_call(WorkQueueManager *queueManager) {
        services::WorkQueueManager::decrementThreadCounter();
    }

    void WorkQueueManager::add_task(model::WorkItem *item) {
        std::lock_guard<std::mutex> lk(WorkQueueManager::work_queue_lock);

        if (item->getRequestType() == enums::request_type::halt_req) {
            int list_size = static_cast<int>(WorkQueueManager::work_queue.size());
            for (int i = 0; i < list_size; i++) {
                if (WorkQueueManager::work_queue[i]->getRequestType() == enums::request_type::dag_disruption) {
                    WorkQueueManager::work_queue.erase(WorkQueueManager::work_queue.begin() + i);
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

        std::sort(WorkQueueManager::work_queue.begin(), WorkQueueManager::work_queue.end(),
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
        std::unique_lock<std::mutex> lk(WorkQueueManager::work_queue_lock, std::defer_lock);
        while (true) {
            WorkQueueManager::current_task.clear();
            WorkQueueManager::thread_counter = 0;

            if (!WorkQueueManager::work_queue.empty()) {
                lk.lock();
                if (WorkQueueManager::work_queue.front()->getRequestType() == enums::request_type::low_complexity) {
                    while (WorkQueueManager::work_queue.front()->getRequestType() ==
                           enums::request_type::low_complexity) {
                        WorkQueueManager::thread_counter++;
                        current_task.push_back(WorkQueueManager::work_queue.front());
                        WorkQueueManager::work_queue.erase(WorkQueueManager::work_queue.begin());
                    }
                } else {
                    WorkQueueManager::thread_counter++;
                    current_task.push_back(WorkQueueManager::work_queue.front());
                    WorkQueueManager::work_queue.erase(WorkQueueManager::work_queue.begin());
                }
                lk.unlock();

                std::vector<std::thread> thread_pool;
                switch (current_task.front()->getRequestType()) {
                    case enums::request_type::low_complexity:
                        for (model::WorkItem *item: current_task)
                            thread_pool.emplace_back(low_comp_allocation_call, item, this);
                        break;
                    case enums::request_type::high_complexity:
                        thread_pool.emplace_back(high_comp_allocation_call, current_task.front(), this);
                        break;
                    case enums::request_type::dag_disruption:
                        thread_pool.emplace_back(dag_disruption_call, current_task.front(), this);
                        break;
                    case enums::request_type::deadline_prempt:
                        thread_pool.emplace_back(deadline_premp_call, current_task.front(), this);
                        break;
                    case enums::request_type::halt_req:
                        thread_pool.emplace_back(halt_call, this);
                        break;
                }

                while (WorkQueueManager::thread_counter > 0) {
                    lk.lock();
                    if (WorkQueueManager::work_queue.front()->getRequestType() == enums::request_type::low_complexity) {
                        while (WorkQueueManager::work_queue.front()->getRequestType() ==
                               enums::request_type::low_complexity) {
                            WorkQueueManager::thread_counter++;
                            thread_pool.emplace_back(low_comp_allocation_call, WorkQueueManager::work_queue.front(),
                                                     this);
                            current_task.push_back(WorkQueueManager::work_queue.front());
                            WorkQueueManager::work_queue.erase(WorkQueueManager::work_queue.begin());
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

    void WorkQueueManager::decrementThreadCounter() {
        WorkQueueManager::thread_counter--;
    }

    double WorkQueueManager::getAverageBitsPerSecond() const {
        return average_bits_per_second;
    }

    void WorkQueueManager::setAverageBitsPerSecond(double averageBitsPerSecond) {
        average_bits_per_second = averageBitsPerSecond;
    }

    double WorkQueueManager::getJitter() const {
        return jitter;
    }

    void WorkQueueManager::setJitter(double jitter) {
        WorkQueueManager::jitter = jitter;
    }

    std::map<std::string, std::shared_ptr<model::BaseResult>> &WorkQueueManager::getOffTotal() {
        return off_total;
    }

    std::map<std::string, std::shared_ptr<model::BaseResult>> &WorkQueueManager::getOffLow() {
        return off_low;
    }

    std::map<std::string, std::shared_ptr<model::BaseResult>> &WorkQueueManager::getOffHigh() {
        return off_high;
    }
} // services