#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"

namespace primebds::commands
{

    static bool cmd_msgtoggle(PrimeBDS &plugin, endstone::CommandSender &sender,
                              const std::vector<std::string> &args)
    {
        auto *player = dynamic_cast<endstone::Player *>(&sender);
        if (!player)
        {
            sender.sendMessage("\u00a7cOnly players can use this command.");
            return true;
        }

        auto user = plugin.db->getOnlineUser(player->getXuid());
        bool enabled = user.has_value() && user->enabled_mt;

        if (enabled)
        {
            plugin.db->updateUser(player->getXuid(), "enabled_mt", "0");
            player->sendMessage("\u00a7cPrivate messages disabled");
        }
        else
        {
            plugin.db->updateUser(player->getXuid(), "enabled_mt", "1");
            player->sendMessage("\u00a7aPrivate messages enabled");
        }
        return true;
    }

    REGISTER_COMMAND(msgtoggle, "Toggles private messages on or off!", cmd_msgtoggle,
                     info.usages = {"/msgtoggle"};
                     info.permissions = {"primebds.command.msgtoggle"};);
} // namespace primebds::commands
