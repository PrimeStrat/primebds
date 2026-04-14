/// @file ipmute.cpp
/// IP mutes a player on the server!

#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"
#include "primebds/utils/moderation.h"
#include "primebds/utils/logging.h"

#include <ctime>
#include <cstdlib>
#include <map>

namespace primebds::commands {

    static bool cmd_ipmute(PrimeBDS &, endstone::CommandSender &,
                        const std::vector<std::string> &);

    REGISTER_COMMAND(ipmute, "IP mutes a player on the server!", cmd_ipmute,
                     info.usages = {"/ipmute <player: player> <duration: int> <unit: string> [reason: message]"};
                     info.permissions = {"primebds.command.ipmute"};);

    static const std::map<std::string, int64_t> ipmute_time_units = {
        {"second", 1}, {"minute", 60}, {"hour", 3600}, {"day", 86400}, {"week", 604800}, {"month", 2592000}, {"year", 31536000}};

    /// IP mutes a player on the server!
    static bool cmd_ipmute(PrimeBDS &plugin, endstone::CommandSender &sender,
                           const std::vector<std::string> &args) {
        if (args.size() < 3) {
            sender.sendMessage("\u00a7cUsage: /ipmute <player> <duration> <unit> [reason]");
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
        auto it = ipmute_time_units.find(unit);
        if (it == ipmute_time_units.end()) {
            sender.sendMessage("\u00a7cInvalid time unit");
            return false;
        }

        int64_t total_secs = utils::safeDuration(duration_number * it->second);
        int64_t expiration = std::time(nullptr) + total_secs;

        std::string reason = "Disruptive Behavior";
        if (args.size() > 3) {
            reason.clear();
            for (size_t i = 3; i < args.size(); ++i) {
                if (i > 3)
                    reason += " ";
                reason += args[i];
            }
        }

        auto user = plugin.db->getUserByName(target_name);
        if (!user) {
            sender.sendMessage("\u00a7cPlayer not found");
            return false;
        }

        auto modlog = plugin.db->getModLog(user->xuid);
        if (modlog && modlog->is_ip_muted) {
            sender.sendMessage("\u00a76Player \u00a7e" + target_name + " \u00a76is already IP muted");
            return false;
        }

        plugin.db->updateModLog(user->xuid, "is_ip_muted", "1");
        plugin.db->updateModLog(user->xuid, "mute_time", std::to_string(expiration));
        plugin.db->updateModLog(user->xuid, "mute_reason", reason);

        std::string time_str = utils::formatTimeRemaining(expiration);
        sender.sendMessage("\u00a76Player \u00a7e" + target_name + " \u00a76was IP muted for \u00a7e\"" +
                           reason + "\" \u00a76for \u00a7e" + time_str);
        utils::log(plugin.getServer(), "\u00a76Player \u00a7e" + target_name + " \u00a76was IP muted by \u00a7e" + sender.getName(), "mod");
        return true;
    }

} // namespace primebds::commands
