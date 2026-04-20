/// @file join.cpp
/// Player join event handler - MOTD, user data, alt detection, nametag formatting.

#include "primebds/handlers/connections/join.h"
#include "primebds/plugin.h"
#include "primebds/utils/config/config_manager.h"
#include "primebds/utils/logging.h"
#include "primebds/utils/permissions/permission_manager.h"

#include <ctime>
#include <string>

namespace primebds::handlers::connections {

    void handleJoinEvent(PrimeBDS &plugin, endstone::PlayerJoinEvent &event) {
        auto &player = event.getPlayer();
        auto &cfg = config::ConfigManager::instance();
        auto conf = cfg.config();
        auto &modules = conf["modules"];

        // Join/leave messages
        bool send_on_connect = modules.value("/join_leave_messages/send_on_connection"_json_pointer, true);
        std::string join_msg = modules.value("/join_leave_messages/join_message"_json_pointer, std::string("{player} has joined"));
        if (send_on_connect) {
            std::string formatted = join_msg;
            auto pos = formatted.find("{player}");
            if (pos != std::string::npos)
                formatted.replace(pos, 8, player.getName());
            event.setJoinMessage(formatted);
        }

        // MOTD
        bool motd_on_connect = modules.value("/message_of_the_day/send_message_of_the_day_on_connect"_json_pointer, false);
        std::string motd = modules.value("/message_of_the_day/message_of_the_day_command"_json_pointer, std::string(""));
        if (motd_on_connect && !motd.empty()) {
            player.sendMessage(motd);
        }

        // Save user data
        std::string xuid = player.getXuid();
        plugin.db->saveUser(xuid, player.getUniqueId().str(), player.getName(),
                            static_cast<int>(player.getPing().count()), player.getDeviceOS(),
                            player.getDeviceId(), static_cast<int64_t>(player.getRuntimeId()),
                            player.getGameVersion());
        plugin.db->updateUser(xuid, "is_afk", "0");
        plugin.db->updateUser(xuid, "last_join", std::to_string(std::time(nullptr)));
        plugin.db->ensureModLog(xuid, player.getName());

        // Alt detection
        auto alts = plugin.db->findAlts(player.getAddress().getHostname(),
                                        player.getDeviceId(), xuid);
        if (!alts.empty()) {
            std::string alt_names;
            for (size_t i = 0; i < alts.size(); ++i) {
                if (i > 0)
                    alt_names += ", ";
                alt_names += alts[i].alt_name;
            }
            std::string msg = "\u00a76Alt Detected: \u00a7e" + player.getName() +
                              " \u00a77-> \u00a78[\u00a77" + alt_names + "\u00a78]";
            utils::log(plugin.getServer(), msg, "mod", "primebds.toggle.altspy");
        }

        // Start session
        plugin.sldb->insertSession(xuid, player.getName(),
                                   player.getAddress().getHostname(),
                                   player.getDeviceOS());

        // Reload custom permissions on the next tick. Capture UUID by value and
        // re-resolve the player so a fast disconnect cannot leave us with a
        // dangling reference (which previously surfaced as std::bad_alloc).
        auto uuid = player.getUniqueId();
        plugin.getServer().getScheduler().runTask(plugin, [&plugin, uuid]() {
            auto *p = plugin.getServer().getPlayer(uuid);
            if (p)
                plugin.reloadCustomPerms(*p);
        });

        // Ban check - suppress join message if banned
        auto mod_log = plugin.db->getModLog(xuid);
        if (mod_log.has_value() && mod_log->is_banned) {
            event.setJoinMessage("");
            return;
        }

        // Warning reminder
        auto warnings = plugin.db->getWarnings(xuid);
        if (!warnings.empty()) {
            auto &latest = warnings.front();
            player.sendMessage("\u00a76Reminder: You were recently warned for \u00a7e" +
                               latest.warn_reason);
        }

        // Rank nametags
        bool rank_nametags = modules.value("/server_messages/rank_meta_nametags"_json_pointer, false);
        if (rank_nametags) {
            auto user = plugin.db->getOnlineUser(xuid);
            if (user.has_value()) {
                auto &pm = permissions::PermissionManager::instance();
                std::string prefix = pm.getPrefix(user->internal_rank);
                std::string suffix = pm.getSuffix(user->internal_rank);
                player.setNameTag(prefix + player.getName() + suffix);
            }
        }

        // Discord relay
        auto online = plugin.getServer().getOnlinePlayers().size();
        auto max_p = plugin.getServer().getMaxPlayers();
        utils::discordRelay("**" + player.getName() + "** has joined the server ***(" +
                                std::to_string(online) + "/" + std::to_string(max_p) + ")***",
                            "connections");
    }

} // namespace primebds::handlers::connections
