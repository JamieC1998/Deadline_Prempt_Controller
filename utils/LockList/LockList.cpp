//
// Created by jamiec on 9/22/22.
//

#include "LockList.h"

using namespace utils;

namespace utils {
    template<typename T>
    void LockList<T>::Push(T value){
        std::lock_guard<std::mutex> lk(mut);
        data.push_back(value);
        condition.notify_one(); //if you have many threads trying to access the data at same time, this will wake one thread only
    }

    template<typename T>
    std::vector<T> LockList<T>::Get(){
        std::unique_lock<std::mutex> lk(mut);
        std::vector<T> vect2;
        vect2.assign(std::begin(data), std::end(data));
        lk.unlock();
        return vect2;
    }

    template<typename T>
    LockList<T>::LockList() {}

    template
    class LockList<std::string>;
} // utils