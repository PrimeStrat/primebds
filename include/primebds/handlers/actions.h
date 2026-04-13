/// @file actions.h
/// Player action handlers - gamemode changes, teleports, deaths, interactions.

#pragma once

#include <endstone/endstone.hpp>

namespace primebds
{
    class PrimeBDS;
}

namespace primebds::handlers
{

    void handleGamemodeEvent(PrimeBDS &plugin, endstone::PlayerGameModeChangeEvent &event);
    void handleTeleportEvent(PrimeBDS &plugin, endstone::PlayerTeleportEvent &event);
    void handleDeathEvent(PrimeBDS &plugin, endstone::PlayerDeathEvent &event);
    void handleInteractEvent(PrimeBDS &plugin, endstone::PlayerInteractActorEvent &event);

} // namespace primebds::handlers
