#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"
#include "primebds/utils/permissions/permission_manager.h"

namespace primebds::commands
{

    static bool cmd_permissions(PrimeBDS &plugin, endstone::CommandSender &sender,
                                const std::vector<std::string> &args)
    {
        if (args.size() < 3)
        {
            sender.sendMessage("\u00a7cUsage: /permissions <player: player> <settrue|setfalse|setneutral> <permission: message>");
            return false;
        }

        std::string target_name = args[0];
        std::string action = args[1];
        for (auto &c : action)
            c = (char)std::tolower(c);
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

        // Look up user by name (supports offline players)
        auto user = plugin.db->getUserByName(target_name);
        if (!user)
        {
            sender.sendMessage("\u00a7cPlayer \u00a7e" + target_name + " \u00a7cnot found");
            return false;
        }

        auto *online_target = plugin.getServer().getPlayer(target_name);

        if (action == "settrue")
        {
            plugin.db->setPermission(user->xuid, perm, true);
            if (online_target)
                plugin.reloadCustomPerms(*online_target);
            sender.sendMessage("\u00a7e" + perm + " \u00a7bpermission for \u00a7e" + target_name + " \u00a7bwas set to \u00a7atrue");
        }
        else if (action == "setfalse")
        {
            plugin.db->setPermission(user->xuid, perm, false);
            if (online_target)
                plugin.reloadCustomPerms(*online_target);
            sender.sendMessage("\u00a7e" + perm + " \u00a7bpermission for \u00a7e" + target_name + " \u00a7bwas set to \u00a7cfalse");
        }
        else if (action == "setneutral")
        {
            plugin.db->removePermission(user->xuid, perm);
            if (online_target)
                plugin.reloadCustomPerms(*online_target);
            sender.sendMessage("\u00a7e" + perm + " \u00a7bpermission for \u00a7e" + target_name + " \u00a7bwas set to \u00a77neutral");
        }
        else
        {
            sender.sendMessage("\u00a7cInvalid action: " + action + ". Use settrue, setfalse, or setneutral");
            return false;
        }

        return true;
    }

    REGISTER_COMMAND(permissions, "Sets the internal permissions for a player!", cmd_permissions,
                     info.usages = {"/permissions <player: player> (settrue|setfalse|setneutral)<set_perm: set_perm> <permission: message>"};
                     info.permissions = {"primebds.command.permissions"};
                     info.aliases = {"perms"};);
} // namespace primebds::commands
