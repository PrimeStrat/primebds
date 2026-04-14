/// @file activity.cpp
/// Lists out session information!

#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"
#include "primebds/utils/time.h"

#include <ctime>
#include <algorithm>

namespace primebds::commands {

    static bool cmd_activity(PrimeBDS &, endstone::CommandSender &,
                        const std::vector<std::string> &);

    REGISTER_COMMAND(activity, "Lists out session information!", cmd_activity,
                     info.usages = {"/activity <player: player> [page: int]"};
                     info.permissions = {"primebds.command.activity"};);

    static std::string formatDuration(int64_t seconds) {
        if (seconds < 60)
            return std::to_string(seconds) + "s";
        int64_t mins = seconds / 60;
        if (mins < 60)
            return std::to_string(mins) + "m";
        int64_t hours = mins / 60;
        if (hours < 24)
            return std::to_string(hours) + "h";
        return std::to_string(hours / 24) + "d";
    }

    /// Lists out session information!
    static bool cmd_activity(PrimeBDS &plugin, endstone::CommandSender &sender,
                             const std::vector<std::string> &args) {
        auto *player = sender.asPlayer();
        if (!player) {
            sender.sendMessage("\u00a7cOnly players can use this command.");
            return true;
        }

        if (args.empty()) {
            sender.sendMessage("\u00a7cUsage: /activity <player> [page]");
            return true;
        }

        for (auto &a : args)
            if (a.find('@') != std::string::npos) {
                sender.sendMessage("\u00a7cTarget selectors are invalid for this command");
                return false;
            }

        std::string target_name = args[0];
        int page = (args.size() > 1) ? std::max(1, std::atoi(args[1].c_str())) : 1;

        auto user = plugin.db->getUserByName(target_name);
        if (!user) {
            sender.sendMessage("No session history found for " + target_name);
            return true;
        }

        int64_t total = plugin.sldb->getTotalPlaytime(user->xuid);
        sender.sendMessage("\u00a7bSession History for \u00a7e" + target_name);
        sender.sendMessage("\u00a7eTotal Playtime: \u00a7f" + formatDuration(total));
        return true;
    }

} // namespace primebds::commands
