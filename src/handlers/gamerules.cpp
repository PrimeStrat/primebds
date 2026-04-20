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

            // Defer per-player permission reload to the next tick. ServerLoadEvent
            // is dispatched synchronously from the command pipeline (e.g. during
            // /reload), and reloadCustomPerms internally dispatches op/deop,
            // which would re-enter the command system and crash Bedrock.
            for (auto *player : plugin.getServer().getOnlinePlayers()) {
                if (!player)
                    continue;
                // Capture UUID by value; look up the player when the task runs.
                // The raw pointer may dangle if the player disconnects before the
                // next tick, which previously manifested as std::bad_alloc when
                // accessing garbage memory inside reloadCustomPerms.
                auto uuid = player->getUniqueId();
                plugin.getServer().getScheduler().runTask(plugin, [&plugin, uuid]() {
                    auto *p = plugin.getServer().getPlayer(uuid);
                    if (!p)
                        return;
                    try {
                        plugin.reloadCustomPerms(*p);
                    } catch (const std::exception &e) {
                        plugin.getLogger().error("Failed to reload perms for {}: {}",
                                                 p->getName(), e.what());
                    } catch (...) {
                        plugin.getLogger().error("Failed to reload perms: unknown error");
                    }
                });
            }
        } catch (const std::exception &e) {
            plugin.getLogger().error("ServerLoadEvent handler failed: {}", e.what());
        } catch (...) {
            plugin.getLogger().error("ServerLoadEvent handler failed: unknown error");
        }
    }

} // namespace primebds::handlers
