/// @file actions.cpp
/// Player action handlers.

#include "primebds/handlers/actions.h"
#include "primebds/plugin.h"
#include "primebds/utils/config/config_manager.h"

namespace primebds::handlers
{

    void handleGamemodeEvent(PrimeBDS &plugin, endstone::PlayerGameModeChangeEvent &event)
    {
        plugin.db->updateUser(event.getPlayer().getXuid(), "gamemode",
                              std::to_string(static_cast<int>(event.getNewGameMode())));
    }

    void handleTeleportEvent(PrimeBDS &plugin, endstone::PlayerTeleportEvent &event)
    {
        auto back_cfg = config::ConfigManager::instance().getModule("back");
        if (back_cfg.value("save_unnatural_teleports", false))
        {
            auto loc = event.getFrom();
            auto encoded = plugin.serverdb->encodeLocation(
                loc.getX(), loc.getY(), loc.getZ(), loc.getDimension().getName(),
                loc.getPitch(), loc.getYaw());
            plugin.serverdb->execute(
                "INSERT OR REPLACE INTO last_warps (xuid, name, location) VALUES (?, ?, ?)",
                {event.getPlayer().getXuid(), event.getPlayer().getName(), encoded});
        }
    }

    void handleDeathEvent(PrimeBDS &plugin, endstone::PlayerDeathEvent &event)
    {
        auto back_cfg = config::ConfigManager::instance().getModule("back");
        if (back_cfg.value("save_death_locations", true))
        {
            auto loc = event.getPlayer().getLocation();
            auto encoded = plugin.serverdb->encodeLocation(
                loc.getX(), loc.getY(), loc.getZ(), loc.getDimension().getName(),
                loc.getPitch(), loc.getYaw());
            plugin.serverdb->execute(
                "INSERT OR REPLACE INTO last_warps (xuid, name, location) VALUES (?, ?, ?)",
                {event.getPlayer().getXuid(), event.getPlayer().getName(), encoded});
        }
    }

    void handleInteractEvent(PrimeBDS &plugin, endstone::PlayerInteractActorEvent &event)
    {
        if (!plugin.gamerules.count("can_interact") || !plugin.gamerules["can_interact"])
            event.setCancelled(true);
    }

} // namespace primebds::handlers
