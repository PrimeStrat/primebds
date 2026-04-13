/// @file leave.h
/// Player leave/quit event handler - session tracking, position saving.

#pragma once

#include <endstone/endstone.hpp>

namespace primebds
{
    class PrimeBDS;
}

namespace primebds::handlers::connections
{

    void handleLeaveEvent(PrimeBDS &plugin, endstone::PlayerQuitEvent &event);

} // namespace primebds::handlers::connections
