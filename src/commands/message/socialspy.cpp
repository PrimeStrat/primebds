#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"

namespace primebds::commands
{

    static bool cmd_socialspy(PrimeBDS &plugin, endstone::CommandSender &sender,
                              const std::vector<std::string> &args)
    {
        auto *player = dynamic_cast<endstone::Player *>(&sender);
        if (!player)
        {
            sender.sendMessage("\u00a7cOnly players can use this command.");
            return true;
        }

        auto user = plugin.db->getOnlineUser(player->getXuid());
        bool enabled = user.has_value() && user->enabled_ss;

        if (enabled)
        {
            plugin.db->updateUser(player->getXuid(), "enabled_ss", "0");
            player->sendMessage("\u00a7cSocialSpy disabled");
        }
        else
        {
            plugin.db->updateUser(player->getXuid(), "enabled_ss", "1");
            player->sendMessage("\u00a7aSocialSpy enabled");
        }
        return true;
    }

    REGISTER_COMMAND(socialspy, "Toggle social spy to see private messages!", cmd_socialspy,
                     info.usages = {"/socialspy"};
                     info.permissions = {"primebds.command.socialspy"};);
} // namespace primebds::commands
