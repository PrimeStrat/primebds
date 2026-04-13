/// @file join.h
/// Player join event handler - MOTD, user data, alt detection, nametag formatting.

#pragma once

#include <endstone/endstone.hpp>

namespace primebds
{
    class PrimeBDS;
}

namespace primebds::handlers::connections
{

    void handleJoinEvent(PrimeBDS &plugin, endstone::PlayerJoinEvent &event);

} // namespace primebds::handlers::connections
