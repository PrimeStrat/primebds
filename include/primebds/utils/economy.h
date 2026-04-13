/// @file economy.h
/// Economy plugin linking utility.

#pragma once

#include <endstone/endstone.hpp>
#include <optional>

namespace primebds::utils
{

    endstone::Plugin *getEconomyLink(endstone::Server &server);

} // namespace primebds::utils
