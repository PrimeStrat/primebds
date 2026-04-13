/// @file target_selector.h
/// Target selector parsing (@a, @p, @r, @s, @n, @e) for Bedrock commands.

#pragma once

#include <endstone/endstone.hpp>
#include <string>
#include <vector>

namespace primebds::utils
{

    std::vector<endstone::Actor *> getMatchingActors(endstone::Server &server,
                                                     const std::string &selector,
                                                     endstone::CommandSender &origin);

    endstone::Player *resolvePlayerTarget(endstone::Server &server,
                                          const std::string &arg,
                                          endstone::CommandSender &origin);

} // namespace primebds::utils
