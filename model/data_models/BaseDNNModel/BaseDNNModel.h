//
// Created by jamiec on 9/26/22.
//

#ifndef CONTROLLER_BASEDNNMODEL_H
#define CONTROLLER_BASEDNNMODEL_H

#include <utility>
#include <vector>
#include <ctime>
#include "../../enums/LayerTypeEnum.h"
#include "../../TileRegion/TileRegion.h"
#include "../../enums/DNN_Type_Enum.h"

namespace model {

    class BaseDNNModel {
    public:
        BaseDNNModel(int layerCount, enums::dnn_type type, std::vector<enums::LayerTypeEnum> layerType,
                     std::vector<TileRegion> inMaps, std::vector<TileRegion> outMaps,
                     std::vector<float> ramReq, std::vector<float> storageReq,
                     std::vector<std::time_t> estimatedProcTime);

        BaseDNNModel();

        int getLayerCount() const;

        void setLayerCount(int layerCount);

        const std::vector<enums::LayerTypeEnum> &getLayerType() const;

        void setLayerType(const std::vector<enums::LayerTypeEnum> &layerType);

        const std::vector<TileRegion> &getInMaps() const;

        void setInMaps(const std::vector<TileRegion> &inMaps);

        const std::vector<TileRegion> &getOutMaps() const;

        void setOutMaps(const std::vector<TileRegion> &outMaps);

        const std::vector<float> &getRamReq() const;

        void setRamReq(const std::vector<float> &ramReq);

        const std::vector<float> &getStorageReq() const;

        void setStorageReq(const std::vector<float> &storageReq);

        const std::vector<std::time_t> &getEstimatedProcTime() const;

        void setEstimatedProcTime(const std::vector<std::time_t> &estimatedProcTime);

        enums::dnn_type getType() const;

        void setType(enums::dnn_type type);

    private:
        static int base_id_counter;
        int LAYER_COUNT = 0;
        int base_dnn_id;
        enums::dnn_type type;
        std::vector<enums::LayerTypeEnum> layer_type;
        std::vector<TileRegion> in_maps;
        std::vector<TileRegion> out_maps;
        std::vector<float> RAM_REQ;
        std::vector<float> STORAGE_REQ;
        std::vector<std::time_t> estimated_proc_time;
    };

} // model

#endif //CONTROLLER_BASEDNNMODEL_H
