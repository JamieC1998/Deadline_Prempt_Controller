//
// Created by jamiec on 10/4/22.
//

#ifndef CONTROLLER_BASECOMMMODEL_H
#define CONTROLLER_BASECOMMMODEL_H

#include <chrono>
#include <string>

namespace model {

    class BaseCommModel {
    private:
        std::chrono::time_point<std::chrono::high_resolution_clock> start_time;
        std::string hostname;

    };

} // model

#endif //CONTROLLER_BASECOMMMODEL_H
