//
// Created by Jamie Cotter on 24/06/2025.
//

#ifndef CONTROLLER_ISPROFILINGSINGLETON_H
#define CONTROLLER_ISPROFILINGSINGLETON_H

#include <mutex>

namespace model {
    class IsProfilingSingleton {


    private:
        IsProfilingSingleton(bool isProfiling) {}

        IsProfilingSingleton(const IsProfilingSingleton &) = delete;

        IsProfilingSingleton &operator=(const IsProfilingSingleton &) = delete;

        static std::mutex mtx; // Mutex for thread safety
        static IsProfilingSingleton *instance;
        std::chrono::time_point<std::chrono::system_clock> ct = std::chrono::system_clock::now();

        bool isProfiling = true;


    public:
        static IsProfilingSingleton *getInstance();

        static bool setIsProfiling(bool profile_flag);
        static bool getIsProfiling();
    };
};


#endif //CONTROLLER_ISPROFILINGSINGLETON_H
