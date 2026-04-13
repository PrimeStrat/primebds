/// @file logging.h
/// Logging utilities and Discord webhook relay.

#pragma once

#include <endstone/endstone.hpp>
#include <string>

namespace primebds::utils
{

    void log(endstone::Server &server, const std::string &message,
             const std::string &type, const std::string &permission = "");
    void discordRelay(const std::string &message, const std::string &type);

} // namespace primebds::utils
