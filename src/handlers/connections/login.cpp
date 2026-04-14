/// @file login.cpp
/// Player login event handler - ban checking and early kick.

#include "primebds/handlers/connections/login.h"
#include "primebds/plugin.h"
#include "primebds/utils/moderation.h"

#include <ctime>

namespace primebds::handlers::connections {

    void handleLoginEvent(PrimeBDS &plugin, endstone::PlayerLoginEvent &event) {
        auto &player = event.getPlayer();
        std::string xuid = player.getXuid();
        std::string ip = player.getAddress().getHostname();
        std::string name = player.getName();

        // Clear crasher patch state on new login
        plugin.crasher_patch_applied.erase(xuid);

        auto now = std::time(nullptr);

        // Check name ban
        if (plugin.serverdb->checkNameBan(name)) {
            auto ban_info = plugin.serverdb->getNameBanInfo(name);
            if (ban_info.has_value()) {
                if (now >= ban_info->banned_time) {
                    plugin.serverdb->removeNameBan(name);
                } else {
                    auto remaining = utils::formatTimeRemaining(ban_info->banned_time);
                    auto msg = utils::formatBanMessage(ban_info->ban_reason, ban_info->banned_time,
                                                       plugin.getServer().getLevel()->getName());
                    event.setKickMessage(msg);
                    event.setCancelled(true);
                    return;
                }
            }
        }

        // Check IP ban
        if (plugin.db->checkIpBan(ip)) {
            auto mod_log = plugin.db->getModLog(xuid);
            if (mod_log.has_value()) {
                if (now >= mod_log->banned_time) {
                    plugin.db->updateModLog(xuid, "is_ip_banned", "0");
                } else {
                    auto remaining = utils::formatTimeRemaining(mod_log->banned_time);
                    auto msg = utils::formatBanMessage("IP Ban - " + mod_log->ban_reason,
                                                       mod_log->banned_time,
                                                       plugin.getServer().getLevel()->getName());
                    event.setKickMessage(msg);
                    event.setCancelled(true);
                    return;
                }
            }
        }

        // Check XUID ban
        auto mod_log = plugin.db->getModLog(xuid);
        if (mod_log.has_value() && mod_log->is_banned) {
            if (now >= mod_log->banned_time) {
                plugin.db->updateModLog(xuid, "is_banned", "0");
                plugin.db->updateModLog(xuid, "ban_reason", "");
                plugin.db->updateModLog(xuid, "banned_time", "0");
            } else {
                auto msg = utils::formatBanMessage(mod_log->ban_reason, mod_log->banned_time,
                                                   plugin.getServer().getLevel()->getName());
                event.setKickMessage(msg);
                event.setCancelled(true);
            }
        }
    }

} // namespace primebds::handlers::connections
