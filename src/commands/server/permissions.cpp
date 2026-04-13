#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"

namespace primebds::commands
{

    static bool cmd_permissions(PrimeBDS &plugin, endstone::CommandSender &sender,
                                const std::vector<std::string> &args)
    {
        if (args.size() < 3)
        {
            sender.sendMessage("\u00a7cUsage: /permissions <settrue|setfalse|setneutral> <player: player> <permission: message>");
            return false;
        }

        std::string action = args[0];
        for (auto &c : action)
            c = (char)std::tolower(c);
        std::string target_name = args[1];
        std::string perm;
        for (size_t i = 2; i < args.size(); ++i)
        {
            if (i > 2)
                perm += " ";
            perm += args[i];
        }

        // Trim perm
        while (!perm.empty() && perm.back() == ' ')
            perm.pop_back();

        auto *target = plugin.getServer().getPlayer(target_name);
        if (!target)
        {
            sender.sendMessage("\u00a7cPlayer \u00a7e" + target_name + " \u00a7cnot found or not online");
            return false;
        }

        if (action == "settrue")
        {
            plugin.db->setPermission(target->getXuid(), perm, true);
            plugin.reloadCustomPerms(*target);
            sender.sendMessage("\u00a7aPermission \u00a7e" + perm + " \u00a7aset to \u00a7etrue \u00a7afor " + target_name);
        }
        else if (action == "setfalse")
        {
            plugin.db->setPermission(target->getXuid(), perm, false);
            plugin.reloadCustomPerms(*target);
            sender.sendMessage("\u00a7aPermission \u00a7e" + perm + " \u00a7aset to \u00a7efalse \u00a7afor " + target_name);
        }
        else if (action == "setneutral")
        {
            plugin.db->removePermission(target->getXuid(), perm);
            plugin.reloadCustomPerms(*target);
            sender.sendMessage("\u00a7aPermission \u00a7e" + perm + " \u00a7aremoved for " + target_name);
        }
        else
        {
            sender.sendMessage("\u00a7cInvalid action: " + action + ". Use settrue, setfalse, or setneutral");
            return false;
        }

        return true;
    }

    REGISTER_COMMAND(permissions, "Set internal permissions for a player!", cmd_permissions,
                     info.usages = {"/permissions <settrue|setfalse|setneutral> <player: player> <permission: message>"};
                     info.permissions = {"primebds.command.permissions"};
                     info.aliases = {"perms"};);
} // namespace primebds::commands
