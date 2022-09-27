//
// Created by jamiec on 9/22/22.
//

#ifndef CONTROLLER_LOCKLIST_H
#define CONTROLLER_LOCKLIST_H

#include <mutex>
#include <queue>
#include <condition_variable>

namespace utils {

    template<typename T>
    class LockList{
    public:
        LockList();

        void Push(T value);

        std::vector<T> Get();

    private:
        std::mutex mut;
        std::vector<T> data; //in your case change this to a std::map
        std::condition_variable condition;
        std::queue<int> ticket_queue;
    };

} // utils

#endif //CONTROLLER_LOCKLIST_H
