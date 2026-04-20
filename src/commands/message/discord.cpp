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
        auto discord = config::ConfigManager::instance().getModule("discord");
        std::string msg = discord.value("command", "");
        if (msg.empty()) {
            sender.sendMessage("\u00a7cNo Discord link has been configured.");
            return true;
        }
        sender.sendMessage(msg);
        return true;
    }

} // namespace primebds::commands
