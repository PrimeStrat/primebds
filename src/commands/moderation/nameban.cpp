/// @file nameban.cpp
/// Bans a player name from the server!

#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"
#include "primebds/utils/logging.h"

#include <ctime>
#include <cstdlib>
#include <map>

namespace primebds::commands {

    static bool cmd_nameban(PrimeBDS &, endstone::CommandSender &,
                        const std::vector<std::string> &);

    REGISTER_COMMAND(nameban, "Bans a player name from the server!", cmd_nameban,
                     info.usages = {"/nameban <name: string> <duration: int> <unit: string> [reason: message]"};
                     info.permissions = {"primebds.command.nameban"};);

    static const std::map<std::string, int64_t> nameban_time_units = {
        {"second", 1}, {"minute", 60}, {"hour", 3600}, {"day", 86400}, {"week", 604800}, {"month", 2592000}, {"year", 31536000}};

    /// Bans a player name from the server!
    static bool cmd_nameban(PrimeBDS &plugin, endstone::CommandSender &sender,
                            const std::vector<std::string> &args) {
        if (args.size() < 3) {
            sender.sendMessage("\u00a7cUsage: /nameban <name> <duration> <unit> [reason]");
            return false;
        }

        std::string name = args[0];
        int dur = std::atoi(args[1].c_str());
        if (dur <= 0) {
            sender.sendMessage("\u00a7cDuration must be positive");
            return false;
        }

        std::string unit = args[2];
        auto it = nameban_time_units.find(unit);
        if (it == nameban_time_units.end()) {
            sender.sendMessage("\u00a7cInvalid time unit");
            return false;
        }

        int64_t duration_secs = dur * it->second;
        std::string reason = "Inappropriate Name";
        if (args.size() > 3) {
            reason.clear();
            for (size_t i = 3; i < args.size(); ++i) {
                if (i > 3)
                    reason += " ";
                reason += args[i];
            }
        }

        if (plugin.serverdb->checkNameBan(name)) {
            sender.sendMessage("\u00a76Name \u00a7e" + name + " \u00a76is already banned");
            return false;
        }

        plugin.serverdb->addNameBan(name, reason, duration_secs);

        // Kick any online player with that name
        auto *target = plugin.getServer().getPlayer(name);
        if (target) {
            target->kick("\u00a7cYour name has been banned: " + reason);
        }

        sender.sendMessage("\u00a76Name \u00a7e" + name + " \u00a76has been banned for \u00a7e\"" + reason + "\"");
        utils::log(plugin.getServer(), "\u00a76Name \u00a7e" + name + " \u00a76was banned by \u00a7e" + sender.getName(), "mod");
        return true;
    }

} // namespace primebds::commands
