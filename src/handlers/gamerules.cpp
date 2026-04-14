/// @file gamerules.cpp
/// Custom gamerule enforcement handlers.

#include "primebds/handlers/gamerules.h"
#include "primebds/plugin.h"
#include "primebds/utils/config/config_manager.h"
#include "primebds/utils/permissions/permission_manager.h"

namespace primebds::handlers {

    void handleBedEnterEvent(PrimeBDS &plugin, endstone::PlayerBedEnterEvent &event) {
        auto it = plugin.gamerules.find("can_sleep");
        if (it != plugin.gamerules.end() && !it->second)
            event.setCancelled(true);
    }

    void handleEmoteEvent(PrimeBDS &plugin, endstone::PlayerEmoteEvent &event) {
        auto it = plugin.gamerules.find("can_emote");
        if (it != plugin.gamerules.end() && !it->second)
            event.setCancelled(true);
    }

    void handleLeavesDecayEvent(PrimeBDS &plugin, endstone::LeavesDecayEvent &event) {
        auto it = plugin.gamerules.find("can_decay_leaves");
        if (it != plugin.gamerules.end() && !it->second)
            event.setCancelled(true);
    }

    void handleSkinChangeEvent(PrimeBDS &plugin, endstone::PlayerSkinChangeEvent &event) {
        auto cfg = config::ConfigManager::instance().getModule("better_chat");
        if (!cfg.value("skin_change_messages", true))
            event.setSkinChangeMessage("");

        auto it = plugin.gamerules.find("can_change_skin");
        if (it != plugin.gamerules.end() && !it->second)
            event.setCancelled(true);
    }

    void handleServerLoadEvent(PrimeBDS &plugin, endstone::ServerLoadEvent &event) {
        // Load permissions on server start/reload
        permissions::PermissionManager::instance().loadPermissions(plugin.getServer());

        // Reload custom permissions for all online players
        for (auto *player : plugin.getServer().getOnlinePlayers()) {
            plugin.reloadCustomPerms(*player);
        }
    }

} // namespace primebds::handlers
