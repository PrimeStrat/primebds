#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"

#include <cstdlib>

namespace primebds::commands
{

    static bool cmd_offlinetp(PrimeBDS &plugin, endstone::CommandSender &sender,
                              const std::vector<std::string> &args)
    {
        auto *player = dynamic_cast<endstone::Player *>(&sender);
        if (!player)
        {
            sender.sendMessage("\u00a7cOnly players can use this command.");
            return true;
        }

        if (args.empty())
        {
            sender.sendMessage("\u00a7cYou must specify a player to teleport to");
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
        if (!user || user->last_logout_pos.empty())
        {
            sender.sendMessage("\u00a7cNo logout record found for " + target_name);
            return true;
        }

        // Parse "x,y,z,dim"
        std::string pos = user->last_logout_pos;
        auto p1 = pos.find(',');
        auto p2 = pos.find(',', p1 + 1);
        auto p3 = pos.find(',', p2 + 1);

        if (p1 == std::string::npos || p2 == std::string::npos)
        {
            sender.sendMessage("\u00a7cLogout location data missing or incomplete for " + target_name);
            return true;
        }

        std::string x = pos.substr(0, p1);
        std::string y = pos.substr(p1 + 1, p2 - p1 - 1);
        std::string z = pos.substr(p2 + 1, (p3 != std::string::npos) ? p3 - p2 - 1 : std::string::npos);

        player->performCommand("tp " + x + " " + y + " " + z);
        sender.sendMessage("Teleported to \u00a7e" + target_name + "\u00a7r's last logout location");
        return true;
    }

    REGISTER_COMMAND(offlinetp, "Teleport to where a player last logged out.", cmd_offlinetp,
                     info.usages = {"/offlinetp <player: player>"};
                     info.permissions = {"primebds.command.offlinetp"};
                     info.default_permission = "op";
                     info.aliases = {"otp"};);
} // namespace primebds::commands
