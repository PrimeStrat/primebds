/// @file discord.cpp
/// Shows the Discord invite link!

#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"
#include "primebds/utils/config/config_manager.h"

namespace primebds::commands {

    static bool cmd_discord(PrimeBDS &, endstone::CommandSender &,
                        const std::vector<std::string> &);

    REGISTER_COMMAND(discord, "Shows the Discord invite link!", cmd_discord,
                     info.usages = {"/discord"};
                     info.permissions = {"primebds.command.discord"};);

    /// Shows the Discord invite link!
    static bool cmd_discord(PrimeBDS &plugin, endstone::CommandSender &sender,
                            const std::vector<std::string> &args) {
        auto &cfg = config::ConfigManager::instance().config();
        std::string link = cfg["modules"]["discord"].value("link", "https://discord.gg/example");
        sender.sendMessage("\u00a7bJoin our Discord: \u00a7e" + link);
        return true;
    }

} // namespace primebds::commands
