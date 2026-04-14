/// @file popup.cpp
/// Sends a popup message to a player!

#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"
#include "primebds/utils/target_selector.h"

namespace primebds::commands {

    static bool cmd_popup(PrimeBDS &, endstone::CommandSender &,
                        const std::vector<std::string> &);

    REGISTER_COMMAND(popup, "Sends a popup message to a player!", cmd_popup,
                     info.usages = {"/popup <player: player> <message: message>"};
                     info.permissions = {"primebds.command.popup"};);

    /// Sends a popup message to a player!
    static bool cmd_popup(PrimeBDS &plugin, endstone::CommandSender &sender,
                          const std::vector<std::string> &args) {
        if (args.size() < 2) {
            sender.sendMessage("\u00a7cUsage: /popup <player> <message>");
            return false;
        }

        std::string msg;
        for (size_t i = 1; i < args.size(); ++i) {
            if (i > 1)
                msg += " ";
            msg += args[i];
        }

        auto targets = utils::getMatchingActors(plugin.getServer(), args[0], sender);
        for (auto *t : targets) {
            if (auto *p = dynamic_cast<endstone::Player *>(t)) {
                p->sendPopup(msg);
            }
        }
        sender.sendMessage("\u00a7aPopup sent to " + std::to_string(targets.size()) + " player(s)");
        return true;
    }

} // namespace primebds::commands
