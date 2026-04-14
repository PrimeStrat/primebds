#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"
#include "primebds/utils/permissions/permission_manager.h"

#include <algorithm>
#include <map>
#include <vector>

namespace primebds::commands
{

    static bool cmd_permissionslist(PrimeBDS &plugin, endstone::CommandSender &sender,
                                    const std::vector<std::string> &args)
    {
        auto &pm = permissions::PermissionManager::instance();

        if (args.empty())
        {
            // Global managed permissions list grouped by prefix
            auto &managed = pm.MANAGED_PERMISSIONS_LIST;
            if (managed.empty())
            {
                sender.sendMessage("\u00a7cNo managed permissions loaded");
                return true;
            }

            std::map<std::string, std::vector<std::string>> grouped;
            for (auto &perm : managed)
            {
                auto dot = perm.find('.');
                std::string prefix = (dot != std::string::npos) ? perm.substr(0, dot) : perm;
                grouped[prefix].push_back(perm);
            }

            sender.sendMessage("\u00a76Server Permissions List");
            for (auto &[prefix, perms] : grouped)
            {
                sender.sendMessage("\u00a7d" + prefix + ":");
                auto sorted = perms;
                std::sort(sorted.begin(), sorted.end());
                for (auto &p : sorted)
                    sender.sendMessage("  \u00a77- \u00a7f" + p);
            }
            return true;
        }

        // Player-specific — show which managed permissions the player has
        std::string target_name = args[0];
        auto *target = plugin.getServer().getPlayer(target_name);
        if (!target)
        {
            sender.sendMessage("\u00a7cPlayer \u00a7e" + target_name + " \u00a7cnot found or not online");
            return false;
        }

        auto &managed = pm.MANAGED_PERMISSIONS_LIST;
        std::map<std::string, std::vector<std::string>> grouped;
        for (auto &perm : managed)
        {
            if (target->hasPermission(perm))
            {
                auto dot = perm.find('.');
                std::string prefix = (dot != std::string::npos) ? perm.substr(0, dot) : perm;
                grouped[prefix].push_back(perm);
            }
        }

        if (grouped.empty())
        {
            sender.sendMessage("\u00a7c" + target_name + " has no managed permissions");
            return true;
        }

        sender.sendMessage("\u00a76" + target_name + "'s Permissions");
        for (auto &[prefix, perms] : grouped)
        {
            sender.sendMessage("\u00a7d" + prefix + ":");
            auto sorted = perms;
            std::sort(sorted.begin(), sorted.end());
            for (auto &p : sorted)
                sender.sendMessage("  \u00a77- \u00a7f" + p);
        }
        return true;
    }

    REGISTER_COMMAND(permissionslist, "View the global or player-specific permissions list!", cmd_permissionslist,
                     info.usages = {
                         "/permissionslist",
                         "/permissionslist <player: player>"};
                     info.permissions = {"primebds.command.permissionslist"}; info.aliases = {"permslist"};);
} // namespace primebds::commands
