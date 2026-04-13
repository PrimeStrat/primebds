#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"

namespace primebds::commands
{

    static bool cmd_alts(PrimeBDS &plugin, endstone::CommandSender &sender,
                         const std::vector<std::string> &args)
    {
        if (args.empty())
        {
            sender.sendMessage("\u00a7cUsage: /alts <player>");
            return false;
        }

        for (auto &a : args)
            if (a.find('@') != std::string::npos)
            {
                sender.sendMessage("\u00a7cTarget selectors are invalid for this command");
                return false;
            }

        std::string target_name = args[0];
        auto user = plugin.db->getUserByName(target_name);
        if (!user)
        {
            sender.sendMessage("\u00a7cPlayer not found");
            return false;
        }

        auto modlog = plugin.db->getModLog(user->xuid);
        if (!modlog)
        {
            sender.sendMessage("\u00a7cNo data for " + target_name);
            return true;
        }

        auto alts = plugin.db->findAlts(modlog->ip_address, user->device_id, user->xuid);
        if (alts.empty())
        {
            sender.sendMessage("\u00a7eNo alts found for " + target_name);
            return true;
        }

        sender.sendMessage("\u00a76Alts for \u00a7e" + target_name + "\u00a76:");
        for (auto &alt : alts)
        {
            sender.sendMessage("\u00a77- \u00a7e" + alt.alt_name + " \u00a78(\u00a77" + alt.alt_xuid + "\u00a78)");
        }
        return true;
    }

    REGISTER_COMMAND(alts, "Check for alternate accounts!", cmd_alts,
                     info.usages = {"/alts <player: player>"};
                     info.permissions = {"primebds.command.alts"};);
} // namespace primebds::commands
