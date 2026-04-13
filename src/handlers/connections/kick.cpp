/// @file kick.cpp
/// Player kick event handler - session cleanup.

#include "primebds/handlers/connections/kick.h"
#include "primebds/plugin.h"

#include <ctime>

namespace primebds::handlers::connections
{

    void handleKickEvent(PrimeBDS &plugin, endstone::PlayerKickEvent &event)
    {
        auto &player = event.getPlayer();
        plugin.sldb->endSession(player.getXuid());
    }

} // namespace primebds::handlers::connections
