/// @file afk.h
/// AFK detection and interval management.

#pragma once

#include <endstone/endstone.hpp>

namespace primebds
{
    class PrimeBDS;
}

namespace primebds::handlers
{

    void initAfkIntervals(PrimeBDS &plugin);
    void stopIntervals(PrimeBDS &plugin);

} // namespace primebds::handlers
