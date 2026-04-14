/// @file gmsp.cpp
/// Sets your game mode to spectator!

#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"
#include "primebds/utils/target_selector.h"

namespace primebds::commands {

    static bool cmd_gmsp(PrimeBDS &, endstone::CommandSender &,
                        const std::vector<std::string> &);

    REGISTER_COMMAND(gmsp, "Sets your game mode to spectator!", cmd_gmsp,
                     info.usages = {"/gmsp [player: player]"};
                     info.permissions = {"primebds.command.gmsp"};);

    /// Sets your game mode to spectator!
    static bool cmd_gmsp(PrimeBDS &plugin, endstone::CommandSender &sender,
                         const std::vector<std::string> &args) {
        if (args.empty()) {
            auto *player = sender.asPlayer();
            if (!player) {
                sender.sendMessage("This command can only be executed by a player");
                return false;
            }
            player->setGameMode(endstone::GameMode::Spectator);
            player->sendMessage("Set own game mode to Spectator");
            return true;
        }
        auto targets = utils::getMatchingActors(plugin.getServer(), args[0], sender);
        for (auto *t : targets) {
            if (auto *p = dynamic_cast<endstone::Player *>(t)) {
                p->setGameMode(endstone::GameMode::Spectator);
                p->sendMessage("Your game mode has been updated to Spectator");
            }
        }
        sender.sendMessage("\u00a7e" + std::to_string(targets.size()) + " \u00a7rplayers were set to Spectator");
        return true;
    }

} // namespace primebds::commands
