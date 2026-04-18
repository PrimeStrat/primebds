/// @file unwarn.cpp
/// Remove a warning or clear all warnings from a player!

#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"
#include "primebds/utils/logging.h"

#include <ctime>
#include <cstdlib>

namespace primebds::commands {

    static bool cmd_unwarn(PrimeBDS &, endstone::CommandSender &,
                        const std::vector<std::string> &);

    REGISTER_COMMAND(unwarn, "Remove a warning or clear all warnings from a player!", cmd_unwarn,
                     info.usages = {
                         "/unwarn <player: player> (clear)<action: unwarn_action> [args: message]",
                         "/unwarn <player: player> [id: int]"};
                     info.permissions = {"primebds.command.unwarn"};);

    /// Remove a warning or clear all warnings from a player!
    static bool cmd_unwarn(PrimeBDS &plugin, endstone::CommandSender &sender,
                           const std::vector<std::string> &args) {
        if (args.empty()) {
            sender.sendMessage("\u00a7cUsage: /unwarn <player> [clear|id]");
            return false;
        }

        std::string target_name = args[0];
        auto user = plugin.db->getUserByName(target_name);
        if (!user) {
            sender.sendMessage("\u00a7cPlayer not found");
            return false;
        }

        if (args.size() == 1) {
            // Remove latest warning
            auto warnings = plugin.db->getWarnings(user->xuid);
            if (warnings.empty()) {
                sender.sendMessage("\u00a7cNo active warnings for \u00a7e" + target_name);
                return false;
            }
            plugin.db->removeWarning(warnings.back().id);
            sender.sendMessage("\u00a76Warning \u00a7eID " + std::to_string(warnings.back().id) +
                               " \u00a76was pardoned for \u00a7e" + target_name);
            utils::log(plugin.getServer(), "\u00a7e" + sender.getName() + " \u00a76pardoned warning \u00a7eID " + std::to_string(warnings.back().id) + " \u00a76for \u00a7e" + target_name, "mod");
            return true;
        }

        std::string action = args[1];

        if (action == "clear") {
            auto warnings = plugin.db->getWarnings(user->xuid);
            for (auto &w : warnings)
                plugin.db->removeWarning(w.id);
            sender.sendMessage("\u00a76All warnings cleared for \u00a7e" + target_name);
            utils::log(plugin.getServer(), "\u00a7e" + sender.getName() + " \u00a76cleared all warnings for \u00a7e" + target_name, "mod");
            return true;
        }

        // Try as warning ID
        int warn_id = std::atoi(action.c_str());
        if (warn_id > 0) {
            plugin.db->removeWarning(warn_id);
            sender.sendMessage("\u00a76Warning \u00a7eID " + std::to_string(warn_id) +
                               " \u00a76pardoned for \u00a7e" + target_name);
            utils::log(plugin.getServer(), "\u00a7e" + sender.getName() + " \u00a76pardoned warning \u00a7eID " + std::to_string(warn_id) + " \u00a76for \u00a7e" + target_name, "mod");
        } else {
            sender.sendMessage("\u00a7cInvalid warning ID");
        }
        return true;
    }

} // namespace primebds::commands
