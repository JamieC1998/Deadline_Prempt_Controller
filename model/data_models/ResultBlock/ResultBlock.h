//
// Created by Jamie Cotter on 04/02/2023.
//

#ifndef CONTROLLER_RESULTBLOCK_H
#define CONTROLLER_RESULTBLOCK_H

#include <map>
#include "../Task/Task.h"

namespace model {

    class ResultBlock {
    public:
        ResultBlock(int n, int m);

        ResultBlock();

        std::map<int, std::shared_ptr<Task>> partitioned_tasks;
        std::map<int, std::shared_ptr<LinkAct>> assembly_upload_windows;
        std::shared_ptr<LinkAct> state_update;

        int getN() const;

        void setN(int n);

        int getM() const;

        void setM(int m);

        const std::string &getAssemblyHost() const;

        void setAssemblyHost(const std::string &assemblyHost);

        bool isCompleted() const;

        void setCompleted(bool completed);

        const std::chrono::time_point<std::chrono::system_clock> &getAssemblyFinTime() const;

        void setAssemblyFinTime(const std::chrono::time_point<std::chrono::system_clock> &assemblyFinTime);

        int getAssemblyHostId() const;

        void setAssemblyHostId(int assemblyHostId);

        const std::chrono::time_point<std::chrono::system_clock> &getStateUpdateFinTime() const;

        void setStateUpdateFinTime(const std::chrono::time_point<std::chrono::system_clock> &stateUpdateFinTime);

        const std::chrono::time_point<std::chrono::system_clock> &getAssemblyStartTime() const;

        void setAssemblyStartTime(const std::chrono::time_point<std::chrono::system_clock> &assemblyStartTime);

        web::json::value convertToJson();

    private:
        int N;
        int M;
        int assembly_host_id;
        std::string assembly_host;
        bool completed = false;
        std::chrono::time_point<std::chrono::system_clock> assembly_fin_time;
        std::chrono::time_point<std::chrono::system_clock> assembly_start_time;
        std::chrono::time_point<std::chrono::system_clock> state_update_fin_time;
    };

} // model

#endif //CONTROLLER_RESULTBLOCK_H
