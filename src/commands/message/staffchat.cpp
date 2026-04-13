#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"

namespace primebds::commands
{

    static bool cmd_staffchat(PrimeBDS &plugin, endstone::CommandSender &sender,
                              const std::vector<std::string> &args)
    {
        auto *player = dynamic_cast<endstone::Player *>(&sender);
        if (!player)
        {
            sender.sendMessage("\u00a7cOnly players can use this command.");
            return true;
        }

        // No arguments = toggle staff chat mode
        if (args.empty())
        {
            auto user = plugin.db->getOnlineUser(player->getXuid());
            bool enabled = user.has_value() && user->enabled_sc;
            if (enabled)
            {
                plugin.db->updateUser(player->getXuid(), "enabled_sc", "0");
                player->sendMessage("\u00a7cStaff chat mode disabled");
            }
            else
            {
                plugin.db->updateUser(player->getXuid(), "enabled_sc", "1");
                player->sendMessage("\u00a7aStaff chat mode enabled");
            }
            return true;
        }

        // With arguments = send staff message
        std::string msg;
        for (size_t i = 0; i < args.size(); ++i)
        {
            if (i > 0)
                msg += " ";
            msg += args[i];
        }

        for (auto *p : plugin.getServer().getOnlinePlayers())
        {
            if (p->hasPermission("primebds.command.staffchat"))
            {
                p->sendMessage("\u00a78[\u00a7cStaff\u00a78] \u00a7e" + player->getName() + "\u00a77: \u00a7f" + msg);
            }
        }
        return true;
    }

    REGISTER_COMMAND(staffchat, "Toggle staff chat or send a staff-only message!", cmd_staffchat,
                     info.usages = {"/staffchat [message: message]"};
                     info.permissions = {"primebds.command.staffchat"};
                     info.aliases = {"sc"};);
} // namespace primebds::commands
