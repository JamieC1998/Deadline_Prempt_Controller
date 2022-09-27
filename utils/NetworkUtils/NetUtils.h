//
// Created by jamiec on 9/22/22.
//

#ifndef CONTROLLER_NETUTILS_H
#define CONTROLLER_NETUTILS_H

#include "../boost_imports.h"

using namespace boost::asio;
using namespace boost::asio::ip;

namespace utils {
    using HostInetInfo = tcp::resolver::iterator;

    class NetUtils {

    private:
        static HostInetInfo queryHostInetInfo();
        static std::string hostIP(unsigned short family);

    public:
        // gets the host IP4 string formatted
        static std::string hostIP4() {
            return hostIP(AF_INET);
        }

        // gets the host IP6 string formatted
        static std::string hostIP6() {
            return hostIP(AF_INET6);
        }

        static std::string hostName() {
            return ip::host_name();
        }
    };
} // utils

#endif //CONTROLLER_NETUTILS_H
