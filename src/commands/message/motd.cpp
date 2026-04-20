/// @file motd.cpp
/// Displays or sets the Message of the Day!

#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"
#include "primebds/utils/config/config_manager.h"

namespace primebds::commands {

    static bool cmd_motd(PrimeBDS &, endstone::CommandSender &,
                        const std::vector<std::string> &);

    REGISTER_COMMAND(motd, "Displays or sets the Message of the Day!", cmd_motd,
                     info.usages = {"/motd [message: message]"};
                     info.permissions = {"primebds.command.motd"};);

    /// Displays or sets the Message of the Day!
    static bool cmd_motd(PrimeBDS &plugin, endstone::CommandSender &sender,
                         const std::vector<std::string> &args) {
        auto &cfg = config::ConfigManager::instance();

        if (!args.empty()) {
            // Set MOTD
            if (!sender.hasPermission("primebds.command.motd.set")) {
                sender.sendMessage("\u00a7cYou don't have permission to set the MOTD");
                return false;
            }
            std::string msg;
            for (size_t i = 0; i < args.size(); ++i) {
                if (i > 0)
                    msg += " ";
                msg += args[i];
            }
            cfg.savePlainText("motd.txt", msg);
            sender.sendMessage("\u00a7aMOTD updated!");
            return true;
        }

        std::string motd = cfg.loadPlainText("motd.txt");
        if (motd.empty())
            motd = "Welcome to the server!";
        sender.sendMessage(motd);
        return true;
    }

} // namespace primebds::commands
