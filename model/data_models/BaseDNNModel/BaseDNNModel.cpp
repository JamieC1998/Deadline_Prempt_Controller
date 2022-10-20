//
// Created by jamiec on 9/26/22.
//

#include "BaseDNNModel.h"

#include <utility>

namespace model {
    int BaseDNNModel::base_id_counter = 0;

    int BaseDNNModel::getLayerCount() const {
        return LAYER_COUNT;
    }

    void BaseDNNModel::setLayerCount(int layerCount) {
        LAYER_COUNT = layerCount;
    }

    void BaseDNNModel::setLayerType(const std::vector<enums::LayerTypeEnum> &layerType) {
        layer_type = layerType;
    }

    const std::vector<TileRegion> &BaseDNNModel::getInMaps() const {
        return in_maps;
    }

    void BaseDNNModel::setInMaps(const std::vector<TileRegion> &inMaps) {
        in_maps = inMaps;
    }

    const std::vector<TileRegion> &BaseDNNModel::getOutMaps() const {
        return out_maps;
    }

    void BaseDNNModel::setOutMaps(const std::vector<TileRegion> &outMaps) {
        out_maps = outMaps;
    }

    const std::vector<float> &BaseDNNModel::getRamReq() const {
        return RAM_REQ;
    }

    void BaseDNNModel::setRamReq(const std::vector<float> &ramReq) {
        RAM_REQ = ramReq;
    }

    const std::vector<float> &BaseDNNModel::getStorageReq() const {
        return STORAGE_REQ;
    }

    void BaseDNNModel::setStorageReq(const std::vector<float> &storageReq) {
        STORAGE_REQ = storageReq;
    }

    enums::dnn_type BaseDNNModel::getType() const {
        return type;
    }

    void BaseDNNModel::setType(enums::dnn_type type) {
        BaseDNNModel::type = type;
    }

    BaseDNNModel::BaseDNNModel(int layerCount, enums::dnn_type type, std::vector<enums::LayerTypeEnum> layerType,
                               std::vector<TileRegion> inMaps, std::vector<TileRegion> outMaps,
                               std::vector<float> ramReq, std::vector<float> storageReq,
                               std::vector<std::chrono::system_clock::duration> estimatedProcTime) : LAYER_COUNT(layerCount), type(type),
                                                                                    layer_type(std::move(layerType)),
                                                                                    in_maps(std::move(inMaps)), out_maps(std::move(outMaps)),
                                                                                    RAM_REQ(std::move(ramReq)),
                                                                                    STORAGE_REQ(std::move(storageReq)),
                                                                                    estimated_proc_time(std::move(estimatedProcTime)),
                                                                                    base_dnn_id(base_id_counter) {
        base_id_counter++;
    }

    BaseDNNModel::BaseDNNModel() {

    }

    std::vector<std::chrono::system_clock::duration> BaseDNNModel::getEstimatedProcTime() const {
        return estimated_proc_time;
    }
} // model