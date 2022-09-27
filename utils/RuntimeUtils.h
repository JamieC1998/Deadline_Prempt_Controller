//
// Created by jamiec on 9/22/22.
//

#ifndef CONTROLLER_RUNTIMEUTILS_H
#define CONTROLLER_RUNTIMEUTILS_H


#include <execinfo.h>
#include <unistd.h>

namespace utils {
    class RuntimeUtils {
    public:
        static void printStackTrace() {
            const int MAX_CALLSTACK = 100;
            void * callstack[MAX_CALLSTACK];
            int frames;

            // get void*'s for all entries on the stack...
            frames = backtrace(callstack, MAX_CALLSTACK);

            // print out all the frames to stderr...
            backtrace_symbols_fd(callstack, frames, STDERR_FILENO);
        }
    };
}

#endif //CONTROLLER_RUNTIMEUTILS_H
