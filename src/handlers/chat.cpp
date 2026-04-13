/// @file chat.cpp
/// Chat event handler - mute system, staff chat, enhanced chat formatting.
/// FIX: Returns early after cancellation (fixes Python UnboundLocalError bug).

#include "primebds/handlers/chat.h"
#include "primebds/plugin.h"
#include "primebds/utils/config/config_manager.h"
#include "primebds/utils/logging.h"
#include "primebds/utils/moderation.h"
#include "primebds/utils/permissions/permission_manager.h"

#include <chrono>
#include <regex>

namespace primebds::handlers
{

    static std::string escapeBraces(const std::string &s)
    {
        std::string out;
        out.reserve(s.size());
        for (char c : s)
        {
            if (c == '{')
                out += "{{";
            else if (c == '}')
                out += "}}";
            else
                out += c;
        }
        return out;
    }

    void handleChatEvent(PrimeBDS &plugin, endstone::PlayerChatEvent &event)
    {
        auto &player = event.getPlayer();
        auto xuid = player.getXuid();

        // Check mute status
        bool user_muted = plugin.db->checkAndUpdateMute(xuid, player.getName());
        auto [ip_muted, ip_mute_time, ip_mute_reason] = plugin.db->checkIpMute(player.getAddress().getHostname());

        // Global mute check
        if (plugin.globalmute == 1 && !player.hasPermission("primebds.globalmute.exempt"))
        {
            player.sendMessage("§cGlobal chat is currently muted by an admin");
            event.setCancelled(true);
            return;
        }

        // Silent mute check
        if (plugin.silentmutes.count(xuid))
        {
            event.setCancelled(true);
            player.sendMessage("§cYour chats are currently disabled");
            return;
        }

        // Mute check
        if (user_muted || ip_muted)
        {
            if (user_muted)
            {
                auto mod = plugin.db->getModLog(xuid);
                if (mod)
                {
                    player.sendMessage(
                        "§6You are currently muted.\n"
                        "§6Expires: §e" +
                        utils::formatTimeRemaining(mod->mute_time) + "\n"
                                                                     "§6Reason: §e" +
                        mod->mute_reason);
                }
            }
            else
            {
                player.sendMessage(
                    "§6You are currently muted.\n"
                    "§6Expires: §e" +
                    utils::formatTimeRemaining(ip_mute_time) + "\n"
                                                               "§6Reason: §e" +
                    ip_mute_reason);
            }
            event.setCancelled(true);
            return;
        }

        auto &cfg = config::ConfigManager::instance();

        // Staff chat check
        auto user = plugin.db->getOnlineUser(xuid);
        if (user && user->enabled_sc)
        {
            auto sc_cfg = cfg.getModule("better_chat");
            auto safe_msg = escapeBraces(event.getMessage());
            auto msg = "§7[Staff] §e" + player.getName() + "§7: §6" + safe_msg;
            plugin.getServer().broadcast(msg, "primebds.command.staffchat");
            event.setCancelled(true);
            return;
        }

        auto chat_cfg = cfg.getModule("better_chat");
        double cooldown = chat_cfg.value("chat_cooldown", 0.0);

        // Chat cooldown
        auto now = std::chrono::duration<double>(
                       std::chrono::steady_clock::now().time_since_epoch())
                       .count();
        auto last_it = plugin.chat_cooldown.find(player.getUniqueId().str());
        double last_time = (last_it != plugin.chat_cooldown.end()) ? last_it->second : 0.0;
        double elapsed = now - last_time;

        if (elapsed < cooldown)
        {
            double remaining = cooldown - elapsed;
            char buf[64];
            snprintf(buf, sizeof(buf), "%.2f", remaining);
            player.sendMessage("§cYou must wait " + std::string(buf) + "s before chatting again!");
            event.setCancelled(true);
            return; // FIX: was missing in Python, caused UnboundLocalError
        }
        plugin.chat_cooldown[player.getUniqueId().str()] = now;

        // Enhanced chat formatting
        bool enhanced = chat_cfg.value("enabled", true);
        if (enhanced && user)
        {
            auto &perms = permissions::PermissionManager::instance();
            auto prefix = escapeBraces(perms.getPrefix(user->internal_rank));
            auto suffix = escapeBraces(perms.getSuffix(user->internal_rank));
            auto name_tag = escapeBraces(player.getNameTag());
            auto safe_msg = escapeBraces(event.getMessage());

            event.setFormat(prefix + name_tag + suffix + "§r" + safe_msg);
        }

        // Discord relay
        utils::discordRelay("**" + player.getName() + "**: " + event.getMessage(), "chat");
    }

} // namespace primebds::handlers
