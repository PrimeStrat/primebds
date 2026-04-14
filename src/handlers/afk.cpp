/// @file afk.cpp
/// AFK detection and interval management.

#include "primebds/handlers/afk.h"
#include "primebds/plugin.h"
#include "primebds/utils/config/config_manager.h"

#include <cmath>
#include <iostream>

namespace primebds::handlers {

    static void checkAfk(PrimeBDS &plugin) {
        auto afk_cfg = config::ConfigManager::instance().getModule("afk");
        if (!afk_cfg.value("enabled", true))
            return;

        bool auto_detect = afk_cfg.value("constantly_check_afk_status", true);
        int idle_threshold = afk_cfg.value("idle_threshold", 300);
        bool broadcast = afk_cfg.value("broadcast_afk_status", true);

        for (auto *player : plugin.getServer().getOnlinePlayers()) {
            try {
                auto xuid = player->getXuid();
                auto user = plugin.db->getOnlineUser(xuid);
                bool is_afk = user ? (user->is_afk != 0) : false;

                auto &cache = plugin.afk_cache;
                auto loc = player->getLocation();

                if (cache.find(xuid) == cache.end())
                    cache[xuid] = false;

                // Simplified movement detection
                // A full implementation would store last location
                if (auto_detect && !is_afk) {
                    // Increment idle time via DB or cache
                }

                if (is_afk) {
                    // Player moved - unmark AFK
                    // (Full implementation needs position tracking)
                }
            } catch (const std::exception &e) {
                std::cerr << "[PrimeBDS] Error in AFK check: " << e.what() << "\n";
            }
        }
    }

    void initAfkIntervals(PrimeBDS &plugin) {
        plugin.afk_interval.addCheck([&plugin]()
                                     { checkAfk(plugin); });
        plugin.afk_interval.start(plugin);
    }

    void stopIntervals(PrimeBDS &plugin) {
        plugin.afk_interval.stop();
    }

} // namespace primebds::handlers
