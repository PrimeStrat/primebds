/// @file removeban.cpp
/// Removes a ban from a player!

#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"
#include "primebds/utils/logging.h"

namespace primebds::commands {

    static bool cmd_removeban(PrimeBDS &, endstone::CommandSender &,
                        const std::vector<std::string> &);

    REGISTER_COMMAND(removeban, "Removes a ban from a player!", cmd_removeban,
                     info.usages = {"/removeban <player: player>"};
                     info.permissions = {"primebds.command.removeban"};
                     info.aliases = {"unban"};);

    /// Removes a ban from a player!
    static bool cmd_removeban(PrimeBDS &plugin, endstone::CommandSender &sender,
                              const std::vector<std::string> &args) {
        if (args.empty()) {
            sender.sendMessage("\u00a7cUsage: /removeban <player>");
            return false;
        }

        for (auto &a : args)
            if (a.find('@') != std::string::npos) {
                sender.sendMessage("\u00a7cTarget selectors are invalid for this command");
                return false;
            }

        std::string target_name = args[0];
        auto user = plugin.db->getUserByName(target_name);
        if (!user) {
            sender.sendMessage("\u00a7cPlayer not found");
            return false;
        }

        auto modlog = plugin.db->getModLog(user->xuid);
        if (!modlog || !modlog->is_banned) {
            sender.sendMessage("\u00a76Player \u00a7e" + target_name + " \u00a76is not banned");
            return false;
        }

        plugin.db->updateModLog(user->xuid, "is_banned", "0");
        plugin.db->updateModLog(user->xuid, "banned_time", "0");
        plugin.db->updateModLog(user->xuid, "ban_reason", "");

        // Also remove IP ban if present
        if (modlog->is_ip_banned) {
            plugin.db->updateModLog(user->xuid, "is_ip_banned", "0");
        }

        sender.sendMessage("\u00a76Player \u00a7e" + target_name + " \u00a76has been unbanned");
        utils::log(plugin.getServer(), "\u00a76Player \u00a7e" + target_name + " \u00a76was unbanned by \u00a7e" + sender.getName(), "mod");
        return true;
    }

} // namespace primebds::commands
