//
// Created by jamiec on 9/26/22.
//

#ifndef CONTROLLER_BASEDNNMODEL_H
#define CONTROLLER_BASEDNNMODEL_H

#include <utility>
#include <vector>
#include <ctime>
#include <chrono>
#include "../../enums/LayerTypeEnum.h"
#include "../../enums/DNN_Type_Enum.h"
#include "../TileRegion/TileRegion.h"

using namespace std::chrono_literals;
namespace model {

    class BaseDNNModel {
    public:
        BaseDNNModel(int layerCount, enums::dnn_type type, std::vector<enums::LayerTypeEnum> layerType,
                     std::vector<TileRegion> inMaps, std::vector<TileRegion> outMaps,
                     std::vector<float> ramReq, std::vector<float> storageReq,
                     std::vector<std::chrono::high_resolution_clock::duration> estimatedProcTime);

        BaseDNNModel();

        int getLayerCount() const;

        void setLayerCount(int layerCount);

        void setLayerType(const std::vector<enums::LayerTypeEnum> &layerType);

        void setInMaps(const std::vector<TileRegion> &inMaps);

        void setOutMaps(const std::vector<TileRegion> &outMaps);

        void setRamReq(const std::vector<float> &ramReq);

        void setStorageReq(const std::vector<float> &storageReq);

        void setType(enums::dnn_type type);

        static int getBaseIdCounter();

        void setBaseDnnSize(unsigned long baseDnnSize);

        int getBaseDnnId() const;

        enums::dnn_type getType() const;

        const std::vector<enums::LayerTypeEnum> &getLayerType() const;

        const std::vector<TileRegion> &getInMaps() const;

        const std::vector<TileRegion> &getOutMaps() const;

        const std::vector<float> &getRamReq() const;

        const std::vector<float> &getStorageReq() const;

        std::vector<std::chrono::high_resolution_clock::duration> getEstimatedProcTime() const;

        unsigned long getBaseDnnSize() const;

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
        std::vector<std::chrono::high_resolution_clock::duration> estimated_proc_time;
        unsigned long baseDNN_size;
    };

} // model

#endif //CONTROLLER_BASEDNNMODEL_H
