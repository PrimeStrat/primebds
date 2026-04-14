/// @file kick.h
/// Player kick event handler - session cleanup.

#pragma once

#include <endstone/endstone.hpp>

namespace primebds {
    class PrimeBDS;
}

namespace primebds::handlers::connections {

    void handleKickEvent(PrimeBDS &plugin, endstone::PlayerKickEvent &event);

} // namespace primebds::handlers::connections
