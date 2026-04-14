/// @file broadcast.cpp
/// Broadcasts a message to the entire server!

#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"
#include "primebds/utils/config/config_manager.h"

namespace primebds::commands {

    static bool cmd_broadcast(PrimeBDS &, endstone::CommandSender &,
                        const std::vector<std::string> &);

    REGISTER_COMMAND(broadcast, "Broadcasts a message to the entire server!", cmd_broadcast,
                     info.usages = {"/broadcast <message: message>"};
                     info.permissions = {"primebds.command.broadcast"};
                     info.aliases = {"bc"};);

    /// Broadcasts a message to the entire server!
    static bool cmd_broadcast(PrimeBDS &plugin, endstone::CommandSender &sender,
                              const std::vector<std::string> &args) {
        if (args.empty()) {
            sender.sendMessage("\u00a7cUsage: /broadcast <message>");
            return false;
        }

        auto &cfg = config::ConfigManager::instance().config();
        std::string prefix = cfg["modules"]["broadcast"].value("prefix", "\u00a76[\u00a7eBroadcast\u00a76]\u00a7r ");
        bool playsound = cfg["modules"]["broadcast"].value("playsound", true);

        std::string msg;
        for (size_t i = 0; i < args.size(); ++i) {
            if (i > 0)
                msg += " ";
            msg += args[i];
        }

        plugin.getServer().broadcastMessage(prefix + msg);
        if (playsound) {
            for (auto *p : plugin.getServer().getOnlinePlayers()) {
                p->performCommand("playsound note.pling @s");
            }
        }
        return true;
    }

} // namespace primebds::commands
