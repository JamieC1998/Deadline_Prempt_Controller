//
// Created by jamiec on 9/22/22.
//

#ifndef CONTROLLER_INTERRUPTUTILS_H
#define CONTROLLER_INTERRUPTUTILS_H

#include <condition_variable>
#include <mutex>
#include <iostream>
#include <signal.h>

static std::condition_variable _condition;
static std::mutex _mutex;

namespace utils {
    class InterruptUtils{
    public:
        static void hookSIGINT() {
            signal(SIGINT, handleUserInterrupt);
        }

        static void handleUserInterrupt(int signal){
            if (signal == SIGINT) {
                std::cout << "SIGINT trapped ..." << '\n';
                _condition.notify_one();
            }
        }

        static void waitForUserInterrupt() {
            std::unique_lock<std::mutex> lock { _mutex };
            _condition.wait(lock);
            std::cout << "user has signaled to interrup program..." << '\n';
            lock.unlock();
        }
    };
}

#endif //CONTROLLER_INTERRUPTUTILS_H
