/// @file ipban.cpp
/// IP bans a player from the server!

#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"
#include "primebds/utils/moderation.h"
#include "primebds/utils/logging.h"

#include <ctime>
#include <cstdlib>
#include <map>

namespace primebds::commands {

    static bool cmd_ipban(PrimeBDS &, endstone::CommandSender &,
                        const std::vector<std::string> &);

    REGISTER_COMMAND(ipban, "IP bans a player from the server!", cmd_ipban,
                     info.usages = {"/ipban <player: player> <duration: int> <unit: string> [reason: message]"};
                     info.permissions = {"primebds.command.ipban"};);

    static const std::map<std::string, int64_t> time_units = {
        {"second", 1}, {"minute", 60}, {"hour", 3600}, {"day", 86400}, {"week", 604800}, {"month", 2592000}, {"year", 31536000}};

    /// IP bans a player from the server!
    static bool cmd_ipban(PrimeBDS &plugin, endstone::CommandSender &sender,
                          const std::vector<std::string> &args) {
        if (args.size() < 3) {
            sender.sendMessage("\u00a7cUsage: /ipban <player> <duration> <unit> [reason]");
            return false;
        }

        for (auto &a : args)
            if (a.find('@') != std::string::npos) {
                sender.sendMessage("\u00a7cTarget selectors are invalid for this command");
                return false;
            }

        std::string target_name = args[0];
        int duration_number = std::atoi(args[1].c_str());
        if (duration_number <= 0) {
            sender.sendMessage("\u00a7cDuration must be positive");
            return false;
        }

        std::string unit = args[2];
        auto it = time_units.find(unit);
        if (it == time_units.end()) {
            sender.sendMessage("\u00a7cInvalid time unit. Use: second, minute, hour, day, week, month, year");
            return false;
        }

        int64_t total_secs = utils::safeDuration(duration_number * it->second);
        int64_t expiration = std::time(nullptr) + total_secs;

        std::string reason = "Negative Behavior";
        if (args.size() > 3) {
            reason.clear();
            for (size_t i = 3; i < args.size(); ++i) {
                if (i > 3)
                    reason += " ";
                reason += args[i];
            }
        }

        auto modlog = plugin.db->getModLog(plugin.db->getUserByName(target_name).value_or(db::User{}).xuid);
        if (!modlog) {
            sender.sendMessage("\u00a7cPlayer not found");
            return false;
        }

        if (modlog->is_ip_banned) {
            sender.sendMessage("\u00a76Player \u00a7e" + target_name + " \u00a76is already IP banned");
            return false;
        }

        plugin.db->updateModLog(modlog->xuid, "is_ip_banned", "1");
        plugin.db->updateModLog(modlog->xuid, "banned_time", std::to_string(expiration));
        plugin.db->updateModLog(modlog->xuid, "ban_reason", reason);

        auto *target = plugin.getServer().getPlayer(target_name);
        std::string time_str = utils::formatTimeRemaining(expiration);
        std::string msg = utils::formatBanMessage(reason, expiration, plugin.getServer().getLevel()->getName());

        if (target) {
            target->kick(msg);
        }

        sender.sendMessage("\u00a76Player \u00a7e" + target_name + " \u00a76was IP banned for \u00a7e\"" +
                           reason + "\" \u00a76for \u00a7e" + time_str);
        utils::log(plugin.getServer(), "\u00a76Player \u00a7e" + target_name + " \u00a76was IP banned by \u00a7e" + sender.getName() + " \u00a76for \u00a7e\"" + reason + "\" \u00a76until \u00a7e" + time_str, "mod");
        return true;
    }

} // namespace primebds::commands
