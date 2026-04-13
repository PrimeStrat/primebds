/// @file leave.cpp
/// Player leave/quit event handler - session tracking, position saving.

#include "primebds/handlers/connections/leave.h"
#include "primebds/plugin.h"
#include "primebds/utils/config/config_manager.h"
#include "primebds/utils/logging.h"

#include <cmath>
#include <ctime>
#include <string>

namespace primebds::handlers::connections
{

    void handleLeaveEvent(PrimeBDS &plugin, endstone::PlayerQuitEvent &event)
    {
        auto &player = event.getPlayer();
        auto &cfg = config::ConfigManager::instance();
        auto conf = cfg.config();
        auto &modules = conf["modules"];

        // Leave message
        bool send_on_connect = modules.value("/join_leave_messages/send_on_connection"_json_pointer, true);
        std::string leave_msg = modules.value("/join_leave_messages/leave_message"_json_pointer, std::string("{player} has left"));
        if (send_on_connect)
        {
            std::string formatted = leave_msg;
            auto pos = formatted.find("{player}");
            if (pos != std::string::npos)
                formatted.replace(pos, 8, player.getName());
            event.setQuitMessage(formatted);
        }

        std::string xuid = player.getXuid();
        int64_t now = std::time(nullptr);

        // Save player data
        plugin.db->updateUser(xuid, "xp", std::to_string(player.getTotalExp()));
        plugin.db->updateUser(xuid, "last_leave", std::to_string(now));
        plugin.db->updateUser(xuid, "is_afk", "0");

        // Check if banned - suppress quit message
        auto mod_log = plugin.db->getModLog(xuid);
        if (mod_log.has_value() && mod_log->is_banned)
        {
            event.setQuitMessage("");
        }
        else
        {
            // End session
            plugin.sldb->endSession(xuid);

            // Save last position
            auto loc = player.getLocation();
            int rx = static_cast<int>(std::round(loc.getX()));
            int ry = static_cast<int>(std::round(loc.getY()));
            int rz = static_cast<int>(std::round(loc.getZ()));
            plugin.db->updateUser(xuid, "last_logout_pos",
                                  std::to_string(rx) + "," + std::to_string(ry) + "," + std::to_string(rz));
            plugin.db->updateUser(xuid, "last_logout_dim", player.getDimension().getName());
        }

        // Discord relay
        auto online = plugin.getServer().getOnlinePlayers().size();
        auto max_p = plugin.getServer().getMaxPlayers();
        utils::discordRelay("**" + player.getName() + "** has left the server ***(" +
                                std::to_string(online > 0 ? online - 1 : 0) + "/" +
                                std::to_string(max_p) + ")***",
                            "connections");
    }

} // namespace primebds::handlers::connections
