//
// Created by jamiec on 9/22/22.
//

#ifndef CONTROLLER_REGISTERDEVICES_H
#define CONTROLLER_REGISTERDEVICES_H


#include <vector>
#include <string>
#include "../../utils/LockList/LockList.h"

namespace services {
    class RegisterDevices {
    private:
        utils::LockList<std::string> device_list;
    public:
        RegisterDevices();

        void register_device(std::string host_name);

        std::vector<std::string> get_devices();

    };
}

#endif //CONTROLLER_REGISTERDEVICES_H
