#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"
#include "primebds/utils/config/config_manager.h"

namespace primebds::commands
{

    static bool cmd_afk(PrimeBDS &plugin, endstone::CommandSender &sender,
                        const std::vector<std::string> &args)
    {
        auto *player = dynamic_cast<endstone::Player *>(&sender);
        if (!player)
        {
            sender.sendMessage("\u00a7cOnly players can use this command.");
            return true;
        }

        auto user = plugin.db->getOnlineUser(player->getXuid());
        bool is_afk = user.has_value() && user->is_afk;

        auto &cfg = config::ConfigManager::instance();
        bool broadcast = cfg.config()["modules"]["afk"].value("broadcast_afk_status", false);

        if (!is_afk)
        {
            plugin.db->updateUser(player->getXuid(), "is_afk", "1");
            player->sendMessage("\u00a77You are now AFK");
            if (broadcast)
                plugin.getServer().broadcastMessage("\u00a7e" + player->getName() + " is now AFK");
        }
        else
        {
            plugin.db->updateUser(player->getXuid(), "is_afk", "0");
            player->sendMessage("\u00a77You are no longer AFK");
            if (broadcast)
                plugin.getServer().broadcastMessage("\u00a7e" + player->getName() + " is no longer AFK");
        }
        return true;
    }

    REGISTER_COMMAND(afk, "Toggles AFK mode for yourself!", cmd_afk,
                     info.usages = {"/afk"};
                     info.permissions = {"primebds.command.afk"};);
} // namespace primebds::commands
