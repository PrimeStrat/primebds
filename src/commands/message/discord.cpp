#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"
#include "primebds/utils/config/config_manager.h"

namespace primebds::commands
{

    static bool cmd_discord(PrimeBDS &plugin, endstone::CommandSender &sender,
                            const std::vector<std::string> &args)
    {
        auto &cfg = config::ConfigManager::instance().config();
        std::string link = cfg["modules"]["discord"].value("link", "https://discord.gg/example");
        sender.sendMessage("\u00a7bJoin our Discord: \u00a7e" + link);
        return true;
    }

    REGISTER_COMMAND(discord, "Shows the Discord invite link!", cmd_discord,
                     info.usages = {"/discord"};
                     info.permissions = {"primebds.command.discord"};);
} // namespace primebds::commands
