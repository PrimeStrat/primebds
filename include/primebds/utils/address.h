/// @file address.h
/// IP address validation and subnet matching utilities.

#pragma once

#include <string>

namespace primebds::utils {

    bool isValidIp(const std::string &ip);
    bool isValidPort(const std::string &port_str);
    std::string stripPort(const std::string &ip_with_port);
    bool sameSubnet(const std::string &ip1, const std::string &ip2, int subnet_mask = 24);

} // namespace primebds::utils
