#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"

namespace primebds::commands
{

    static bool cmd_modspy(PrimeBDS &plugin, endstone::CommandSender &sender,
                           const std::vector<std::string> &args)
    {
        auto *player = sender.asPlayer();
        if (!player)
        {
            sender.sendMessage("\u00a7cOnly players can use this command.");
            return true;
        }

        auto user = plugin.db->getOnlineUser(player->getXuid());
        bool enabled = user.has_value() && user->enabled_ms;

        if (enabled)
        {
            plugin.db->updateUser(player->getXuid(), "enabled_ms", "0");
            player->sendMessage("\u00a7cModSpy disabled");
        }
        else
        {
            plugin.db->updateUser(player->getXuid(), "enabled_ms", "1");
            player->sendMessage("\u00a7aModSpy enabled");
        }
        return true;
    }

    REGISTER_COMMAND(modspy, "Toggle moderation spy notifications!", cmd_modspy,
                     info.usages = {"/modspy"};
                     info.permissions = {"primebds.command.modspy"};);
} // namespace primebds::commands
