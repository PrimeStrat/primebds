#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"
#include "primebds/utils/config/config_manager.h"

namespace primebds::commands
{

    static bool cmd_motd(PrimeBDS &plugin, endstone::CommandSender &sender,
                         const std::vector<std::string> &args)
    {
        if (!args.empty())
        {
            // Set MOTD
            if (!sender.hasPermission("primebds.command.motd.set"))
            {
                sender.sendMessage("\u00a7cYou don't have permission to set the MOTD");
                return false;
            }
            std::string msg;
            for (size_t i = 0; i < args.size(); ++i)
            {
                if (i > 0)
                    msg += " ";
                msg += args[i];
            }
            auto &cfg = config::ConfigManager::instance();
            auto &conf = cfg.config();
            conf["modules"]["motd"]["message"] = msg;
            cfg.save();
            sender.sendMessage("\u00a7aMOTD updated!");
            return true;
        }

        auto &cfg = config::ConfigManager::instance().config();
        std::string motd = cfg["modules"]["motd"].value("message", "Welcome to the server!");
        sender.sendMessage(motd);
        return true;
    }

    REGISTER_COMMAND(motd, "Displays or sets the Message of the Day!", cmd_motd,
                     info.usages = {"/motd [message: message]"};
                     info.permissions = {"primebds.command.motd"};);
} // namespace primebds::commands
