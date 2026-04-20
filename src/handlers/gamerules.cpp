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
        // Wrapped in try/catch so a permission/reload failure can never propagate
        // out of the event dispatcher (which would log "std::bad_alloc" and abort
        // the rest of the listeners). ServerLoadEvent is not cancellable, so the
        // worst case is permissions don't refresh on this load.
        try {
            permissions::PermissionManager::instance().loadPermissions(plugin.getServer());

            for (auto *player : plugin.getServer().getOnlinePlayers()) {
                try {
                    plugin.reloadCustomPerms(*player);
                } catch (const std::exception &e) {
                    plugin.getLogger().error("Failed to reload perms for {}: {}",
                                             player ? player->getName() : "<null>", e.what());
                }
            }
        } catch (const std::exception &e) {
            plugin.getLogger().error("ServerLoadEvent handler failed: {}", e.what());
        } catch (...) {
            plugin.getLogger().error("ServerLoadEvent handler failed: unknown error");
        }
    }

} // namespace primebds::handlers
