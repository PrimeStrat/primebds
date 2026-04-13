/// @file login.h
/// Player login event handler - ban checking and early kick.

#pragma once

#include <endstone/endstone.hpp>

namespace primebds
{
    class PrimeBDS;
}

namespace primebds::handlers::connections
{

    void handleLoginEvent(PrimeBDS &plugin, endstone::PlayerLoginEvent &event);

} // namespace primebds::handlers::connections
