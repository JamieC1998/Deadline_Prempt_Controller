//
// Created by jamiec on 9/22/22.
//

#include "RegisterDevices.h"

#include <utility>

namespace services {
    std::vector<std::string> RegisterDevices::get_devices() {
        return device_list.Get();
    }

    void RegisterDevices::register_device(std::string host_name) {
        device_list.Push(std::move(host_name));
    }

    RegisterDevices::RegisterDevices() {}
}