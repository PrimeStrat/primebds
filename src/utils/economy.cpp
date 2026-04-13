/// @file economy.cpp
/// Economy plugin linking utility.

#include "primebds/utils/economy.h"

namespace primebds::utils
{

    endstone::Plugin *getEconomyLink(endstone::Server &server)
    {
        return server.getPluginManager().getPlugin("umoney");
    }

} // namespace primebds::utils
