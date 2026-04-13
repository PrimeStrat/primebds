#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"

namespace primebds::commands
{

    static bool cmd_permissionslist(PrimeBDS &plugin, endstone::CommandSender &sender,
                                    const std::vector<std::string> &args)
    {
        if (args.empty())
        {
            // Global permissions list
            auto all = plugin.db->getAllInternalPermissions();
            if (all.empty())
            {
                sender.sendMessage("\u00a7cNo custom permissions set");
                return true;
            }
            sender.sendMessage("\u00a7a--- Global Permissions ---");
            for (auto &[xuid, perms] : all)
            {
                auto user = plugin.db->getUserByXuid(xuid);
                std::string name = user ? user->name : xuid;
                for (auto &[perm, value] : perms)
                {
                    sender.sendMessage("\u00a7e" + name + ": \u00a7b" + perm + " = \u00a7r" + value);
                }
            }
            return true;
        }

        // Player-specific
        std::string target_name = args[0];
        auto *target = plugin.getServer().getPlayer(target_name);
        if (!target)
        {
            sender.sendMessage("\u00a7cPlayer \u00a7e" + target_name + " \u00a7cnot found or not online");
            return false;
        }

        auto perms = plugin.db->getInternalPermissions(target->getXuid());
        if (perms.empty())
        {
            sender.sendMessage("\u00a7c" + target_name + " has no custom permissions");
            return true;
        }

        sender.sendMessage("\u00a7a--- " + target_name + "'s Permissions ---");
        for (auto &[perm, value] : perms)
        {
            sender.sendMessage("\u00a7b" + perm + " = \u00a7r" + value);
        }
        return true;
    }

    REGISTER_COMMAND(permissionslist, "View custom permissions!", cmd_permissionslist,
                     info.usages = {
                         "/permissionslist",
                         "/permissionslist <player: player>"};
                     info.permissions = {"primebds.command.permissionslist"}; info.aliases = {"permslist"};);
} // namespace primebds::commands
