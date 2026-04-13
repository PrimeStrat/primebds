/// @file address.cpp
/// IP address validation and subnet utilities.

#include "primebds/utils/address.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <regex>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <arpa/inet.h>
#include <sys/socket.h>
#endif

namespace primebds::utils
{

    bool isValidIp(const std::string &ip)
    {
        // Try IPv4
        struct in_addr addr4;
        if (inet_pton(AF_INET, ip.c_str(), &addr4) == 1)
            return true;

        // Try IPv6
        struct in6_addr addr6;
        if (inet_pton(AF_INET6, ip.c_str(), &addr6) == 1)
            return true;

        // Try hostname (RFC 1123)
        static const std::regex hostname_re(
            R"(^(?=.{1,253}$)(?!-)[A-Za-z0-9-]{1,63}(?<!-)(?:\.(?!-)[A-Za-z0-9-]{1,63}(?<!-))*\.?$)");
        return std::regex_match(ip, hostname_re);
    }

    bool isValidPort(const std::string &port_str)
    {
        if (port_str.empty())
            return false;
        for (char c : port_str)
        {
            if (!std::isdigit(static_cast<unsigned char>(c)))
                return false;
        }
        int port = std::stoi(port_str);
        return port >= 1 && port <= 65535;
    }

    std::string stripPort(const std::string &ip_with_port)
    {
        if (ip_with_port.empty())
            return {};
        auto pos = ip_with_port.find(':');
        return pos != std::string::npos ? ip_with_port.substr(0, pos) : ip_with_port;
    }

    bool sameSubnet(const std::string &ip1, const std::string &ip2, int subnet_mask)
    {
        auto clean1 = stripPort(ip1);
        auto clean2 = stripPort(ip2);

        struct in_addr a1, a2;
        if (inet_pton(AF_INET, clean1.c_str(), &a1) != 1)
            return false;
        if (inet_pton(AF_INET, clean2.c_str(), &a2) != 1)
            return false;

        uint32_t mask = (subnet_mask == 0) ? 0 : (~0u << (32 - subnet_mask));
        mask = htonl(mask);

        return (a1.s_addr & mask) == (a2.s_addr & mask);
    }

} // namespace primebds::utils
