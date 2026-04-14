/// @file check.cpp
/// Checks a player's client info!

#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"
#include "primebds/utils/time.h"

#include <cmath>
#include <ctime>
#include <string>

namespace primebds::commands {

    static bool cmd_check(PrimeBDS &, endstone::CommandSender &,
                        const std::vector<std::string> &);

    REGISTER_COMMAND(check, "Checks a player's client info!", cmd_check,
                     info.usages = {"/check <player: player> (info|mod|network|world)[info: info]"};
                     info.permissions = {"primebds.command.check"};
                     info.default_permission = "op";
                     info.aliases = {"seen"};);

    /// Formats a unix timestamp expiry into a human-readable remaining time string
    static std::string formatTimeRemaining(int64_t expiry_time) {
        if (expiry_time <= 0)
            return "Permanent";

        int64_t now = static_cast<int64_t>(std::time(nullptr));
        int64_t remaining = expiry_time - now;
        if (remaining <= 0)
            return "Expired";

        int64_t days = remaining / 86400;
        int64_t hours = (remaining % 86400) / 3600;
        int64_t minutes = (remaining % 3600) / 60;

        std::string result;
        if (days > 0)
            result += std::to_string(days) + "d ";
        if (hours > 0)
            result += std::to_string(hours) + "h ";
        result += std::to_string(minutes) + "m";
        return result;
    }

    /// Converts a GameMode enum to a display name
    static std::string gameModeName(endstone::GameMode mode) {
        switch (mode) {
        case endstone::GameMode::Survival:
            return "Survival";
        case endstone::GameMode::Creative:
            return "Creative";
        case endstone::GameMode::Adventure:
            return "Adventure";
        case endstone::GameMode::Spectator:
            return "Spectator";
        default:
            return "Unknown";
        }
    }

    /// Checks a player's client info!
    static bool cmd_check(PrimeBDS &plugin, endstone::CommandSender &sender,
                          const std::vector<std::string> &args) {
        if (args.empty()) {
            sender.sendMessage("\u00a7cUsage: /check <player> [info|mod|network|world]");
            return false;
        }

        for (auto &arg : args) {
            if (arg.find('@') != std::string::npos) {
                sender.sendMessage("\u00a7cTarget selectors are invalid for this command");
                return false;
            }
        }

        std::string player_name = args[0];
        // Strip quotes from player name
        if (player_name.size() >= 2 && player_name.front() == '"' && player_name.back() == '"')
            player_name = player_name.substr(1, player_name.size() - 2);

        std::string filter = "info";
        if (args.size() > 1) {
            filter = args[1];
            for (auto &c : filter)
                c = static_cast<char>(std::tolower(c));
        }

        auto *target = plugin.getServer().getPlayer(player_name);
        auto user = plugin.db->getUserByName(player_name);

        if (!user.has_value() && !target) {
            sender.sendMessage("Player \u00a7e" + player_name + "\u00a7c not found in database.");
            return false;
        }

        // Resolve common fields
        auto &u = user.value();
        std::string status = target ? "\u00a7aOnline" : "\u00a7cOffline";
        std::string ping_str = target
                                   ? std::to_string(static_cast<int>(target->getPing().count())) + "ms"
                                   : std::to_string(u.ping) + "ms \u00a77[Last Recorded\u00a77]";

        // Format join/leave timestamps
        std::string join_time = utils::convertToTimezone(u.last_join, "EST");
        std::string leave_time = "N/A";
        if (u.last_leave > 946684800) // year 2000 sanity check
            leave_time = utils::convertToTimezone(u.last_leave, "EST");

        if (filter == "info") {
            std::string msg = "\u00a7bPlayer Database Information:\n";
            msg += "\u00a77- \u00a7eName: \u00a7f" + u.name + " \u00a78[" + status + "\u00a78]\n";
            msg += "\u00a77- \u00a7eXUID: \u00a7f" + u.xuid + "\n";
            msg += "\u00a77- \u00a7eUUID: \u00a7f" + u.uuid + "\n";
            msg += "\u00a77- \u00a7eUnique ID: \u00a7f" + std::to_string(u.unique_id) + "\n";
            msg += "\u00a77- \u00a7eInternal Rank: \u00a7f" + u.internal_rank + "\n";
            msg += "\u00a77- \u00a7eDevice OS: \u00a7f" + u.device_os + "\n";
            msg += "\u00a77- \u00a7eDevice ID: \u00a7f" + u.device_id + "\n";
            msg += "\u00a77- \u00a7eClient Version: \u00a7f" + u.client_ver + "\n";
            msg += "\u00a77- \u00a7ePing: \u00a7f" + ping_str + "\n";
            msg += "\u00a77- \u00a7eLast Join: \u00a7f" + join_time + "\n";
            msg += "\u00a77- \u00a7eLast Leave: \u00a7f" + leave_time;
            sender.sendMessage(msg);
        } else if (filter == "mod") {
            auto mod = plugin.db->getModLog(u.xuid);
            std::string msg = "\u00a76Player Mod Information:\n";
            msg += "\u00a77- \u00a7eName: \u00a7f" + u.name + " \u00a78[" + status + "\u00a78]\n";
            msg += "\u00a77- \u00a7eRank: \u00a7f" + u.internal_rank + "\n";

            if (mod.has_value()) {
                // Ban status
                msg += "\u00a77- \u00a7eBanned: \u00a7f" + std::string(mod->is_banned ? "true" : "false") +
                       " \u00a78[\u00a77IP: " + std::string(mod->is_ip_banned ? "true" : "false") + "\u00a78]\n";
                if (mod->is_banned) {
                    msg += "\u00a77  - \u00a7eBan Reason: \u00a7f" + mod->ban_reason + "\n";
                    msg += "\u00a77  - \u00a7eBan Expires: \u00a7f" + formatTimeRemaining(mod->banned_time) + "\n";
                }

                // Name ban status
                bool name_banned = plugin.serverdb->checkNameBan(u.name);
                msg += "\u00a77- \u00a7eName Banned: \u00a7f" + std::string(name_banned ? "true" : "false") + "\n";
                if (name_banned) {
                    auto name_ban = plugin.serverdb->getNameBanInfo(u.name);
                    if (name_ban.has_value()) {
                        msg += "\u00a77  - \u00a7eBan Reason: \u00a7f" + name_ban->ban_reason + "\n";
                        msg += "\u00a77  - \u00a7eBan Expires: \u00a7f" + formatTimeRemaining(name_ban->banned_time) + "\n";
                    }
                }

                // Mute status
                msg += "\u00a77- \u00a7eMuted: \u00a7f" + std::string(mod->is_muted ? "true" : "false") +
                       " \u00a78[\u00a77IP: " + std::string(mod->is_ip_muted ? "true" : "false") + "\u00a78]\n";
                if (mod->is_muted) {
                    msg += "\u00a77  - \u00a7eMute Reason: \u00a7f" + mod->mute_reason + "\n";
                    msg += "\u00a77  - \u00a7eMute Expires: \u00a7f" + formatTimeRemaining(mod->mute_time) + "\n";
                }

                // Warnings
                auto warnings = plugin.db->getWarnings(u.xuid);
                // Find latest active warning (warn_time > now or warn_time <= 0 for permanent)
                int64_t now = static_cast<int64_t>(std::time(nullptr));
                const db::Warn *active_warn = nullptr;
                for (auto &w : warnings) {
                    if (w.warn_time <= 0 || w.warn_time > now) {
                        active_warn = &w;
                        break;
                    }
                }
                msg += "\u00a77- \u00a7eWarned: \u00a7f" + std::string(active_warn ? "true" : "false") + "\n";
                if (active_warn) {
                    msg += "\u00a77  - \u00a7eWarn Reason: \u00a7f" + active_warn->warn_reason + "\n";
                    msg += "\u00a77  - \u00a7eWarn Expires: \u00a7f" + formatTimeRemaining(active_warn->warn_time) + "\n";
                }
            }
            sender.sendMessage(msg);
        } else if (filter == "network") {
            std::string ip;
            if (target) {
                ip = target->getAddress().getHostname();
            } else {
                auto mod = plugin.db->getModLog(u.xuid);
                ip = (mod.has_value() && !mod->ip_address.empty()) ? mod->ip_address : "(unknown)";
            }

            std::string msg = "\u00a7dPlayer Network Information:\n";
            msg += "\u00a77- \u00a7eName: \u00a7f" + u.name + " \u00a78[" + status + "\u00a78]\n";
            msg += "\u00a77- \u00a7eIP: \u00a7f" + ip;
            sender.sendMessage(msg);
        } else if (filter == "world") {
            if (!target) {
                sender.sendMessage("\u00a7cPlayer must be online to check world data");
                return false;
            }

            auto loc = target->getLocation();
            auto tags = target->getScoreboardTags();
            std::string tags_str;
            if (tags.empty()) {
                tags_str = "(none)";
            } else {
                for (size_t i = 0; i < tags.size(); ++i) {
                    if (i > 0)
                        tags_str += ", ";
                    tags_str += tags[i];
                }
            }

            std::string msg = "\u00a7aPlayer World Information:\n";
            msg += "\u00a77- \u00a7eName: \u00a7f" + u.name + " \u00a78[" + status + "\u00a78]\n";
            msg += "\u00a77- \u00a7eRuntime ID: \u00a7f" + std::to_string(target->getRuntimeId()) + "\n";
            msg += "\u00a77- \u00a7eGamemode: \u00a7f" + gameModeName(target->getGameMode()) + "\n";
            msg += "\u00a77- \u00a7eLocation: \u00a7fx: " + std::to_string(loc.getBlockX()) +
                   " \u00a77/ \u00a7fy: " + std::to_string(loc.getBlockY()) +
                   " \u00a77/ \u00a7fz: " + std::to_string(loc.getBlockZ()) + "\n";
            msg += "\u00a77- \u00a7eRotation: \u00a7fyaw: " +
                   std::to_string(static_cast<int>(std::round(loc.getYaw() * 100.0f)) / 100.0) +
                   " \u00a77/ \u00a7fpitch: " +
                   std::to_string(static_cast<int>(std::round(loc.getPitch() * 100.0f)) / 100.0) + "\n";
            msg += "\u00a77- \u00a7eDimension: \u00a7f" + target->getDimension().getName() + "\n";
            msg += "\u00a77- \u00a7eHealth: \u00a7f" + std::to_string(target->getHealth()) +
                   "/" + std::to_string(target->getMaxHealth()) + "\n";
            msg += "\u00a77- \u00a7eTotal XP: \u00a7f" + std::to_string(target->getTotalExp()) + "\n";
            msg += "\u00a77- \u00a7eGrounded: \u00a7f" + std::string(target->isOnGround() ? "true" : "false") + "\n";
            msg += "\u00a77- \u00a7eIn Lava: \u00a7f" + std::string(target->isInLava() ? "true" : "false") + "\n";
            msg += "\u00a77- \u00a7eIn Water: \u00a7f" + std::string(target->isInWater() ? "true" : "false") + "\n";
            msg += "\u00a77- \u00a7eIs OP: \u00a7f" + std::string(target->isOp() ? "true" : "false") + "\n";
            msg += "\u00a77- \u00a7eCan Fly: \u00a7f" + std::string(target->getAllowFlight() ? "true" : "false") + "\n";
            msg += "\u00a77- \u00a7eTags: \u00a7f" + tags_str;
            sender.sendMessage(msg);
        } else {
            sender.sendMessage("\u00a7cInvalid filter type '" + filter + "'");
            return false;
        }

        return true;
    }

} // namespace primebds::commands
