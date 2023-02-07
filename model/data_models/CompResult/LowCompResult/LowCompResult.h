//
// Created by Jamie Cotter on 05/02/2023.
//

#ifndef CONTROLLER_LOWCOMPRESULT_H
#define CONTROLLER_LOWCOMPRESULT_H

#include <string>
#include "../../LinkAct/LinkAct.h"
#include "../BaseCompResult/BaseCompResult.h"
#include "../../Task/Task.h"

namespace model {

    class LowCompResult: public BaseCompResult {

    public:

        LowCompResult(const std::string &dnnId, enums::dnn_type dnnType, const std::string &srcHost,
                      const std::chrono::time_point<std::chrono::system_clock> &deadline,
                      const std::chrono::time_point<std::chrono::system_clock> &estimatedStart,
                      const std::chrono::time_point<std::chrono::system_clock> &estimatedFinish,
                      const std::shared_ptr<LinkAct> &uploadData);

        LowCompResult(const std::string &dnnId, enums::dnn_type dnnType, const std::string &srcHost,
                      const std::chrono::time_point<std::chrono::system_clock> &deadline,
                      const std::shared_ptr<LinkAct> &uploadData);

        LowCompResult();

        const std::string &getSrcHost() const;

        void setSrcHost(const std::string &srcHost);

        const std::chrono::time_point<std::chrono::system_clock> &getDeadline() const;

        void setDeadline(const std::chrono::time_point<std::chrono::system_clock> &deadline);

        const std::chrono::time_point<std::chrono::system_clock> &getEstimatedStart() const;

        void setEstimatedStart(const std::chrono::time_point<std::chrono::system_clock> &estimatedStart);

        const std::chrono::time_point<std::chrono::system_clock> &getEstimatedFinish() const;

        void setEstimatedFinish(const std::chrono::time_point<std::chrono::system_clock> &estimatedFinish);

        const std::shared_ptr<LinkAct> &getUploadData() const;

        void setUploadData(const std::shared_ptr<LinkAct> &uploadData);

        const std::shared_ptr<model::Task> &getTask() const;

        void setTask(const std::shared_ptr<model::Task> &task);

        web::json::value convertToJson();

    private:
        std::string srcHost;
        std::chrono::time_point<std::chrono::system_clock> deadline;
        std::chrono::time_point<std::chrono::system_clock> estimatedStart;
        std::chrono::time_point<std::chrono::system_clock> estimatedFinish;
        std::shared_ptr<model::Task> task;
        std::shared_ptr<LinkAct> upload_data;
    };

} // model

#endif //CONTROLLER_LOWCOMPRESULT_H
