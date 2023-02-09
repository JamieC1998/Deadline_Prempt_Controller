//
// Created by jamiec on 9/27/22.
//

#ifndef CONTROLLER_HIGHCOMPRESULT_H
#define CONTROLLER_HIGHCOMPRESULT_H

#include <ctime>
#include <string>
#include <map>
#include "../../../enums/DNN_Type_Enum.h"
#include "../../Task/Task.h"
#include "../../ResultBlock/ResultBlock.h"
#include "../BaseCompResult/BaseCompResult.h"

namespace model {

    class HighCompResult: public BaseCompResult{
    public:
        HighCompResult(const std::string &dnnId, enums::dnn_type dnnType, std::string srcHost,
                       const std::chrono::time_point<std::chrono::system_clock> &deadline,
                       const std::chrono::time_point<std::chrono::system_clock> &estimatedStart,
                       std::string startingConvidx, std::shared_ptr<LinkAct> uploadData);

        const std::string &getSrcHost() const;

        void setSrcHost(const std::string &srcHost);

        std::chrono::time_point<std::chrono::system_clock> getDeadline() const;

        void setDeadline(std::chrono::time_point<std::chrono::system_clock> deadline);

        std::chrono::time_point<std::chrono::system_clock> getEstimatedStart() const;

        void setEstimatedStart(std::chrono::time_point<std::chrono::system_clock> estimatedStart);

        std::chrono::time_point<std::chrono::system_clock> getEstimatedFinish() const;

        void setEstimatedFinish(std::chrono::time_point<std::chrono::system_clock> estimatedFinish);

        int getLastCompleteConvIdx() const;

        void setLastCompleteConvIdx(int currentConvix);

        const std::string &getStartingConvidx() const;

        void setStartingConvidx(const std::string &startingConvidx);

        const std::shared_ptr<LinkAct> &getUploadData() const;

        void setUploadData(const std::shared_ptr<LinkAct> &uploadData);

        void resetUploadData();

        web::json::value convertToJson();

        std::map<std::string, std::shared_ptr<ResultBlock>> tasks;

        const std::string &getVersion() const;

        void setVersion(const std::string &version);

    private:
        std::string srcHost;
        std::chrono::time_point<std::chrono::system_clock> deadline;
        std::chrono::time_point<std::chrono::system_clock> estimatedStart;
        std::chrono::time_point<std::chrono::system_clock> estimatedFinish;

        std::string starting_convidx;
        int lastCompleteConvidx = -1;
        std::shared_ptr<LinkAct> upload_data;
        std::string version = std::to_string(int(std::chrono::system_clock::now().time_since_epoch().count()));
    };

} // model

#endif //CONTROLLER_HIGHCOMPRESULT_H
