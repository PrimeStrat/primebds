/// @file gms.cpp
/// Sets your game mode to survival!

#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"
#include "primebds/utils/target_selector.h"

namespace primebds::commands {

    static bool cmd_gms(PrimeBDS &, endstone::CommandSender &,
                        const std::vector<std::string> &);

    REGISTER_COMMAND(gms, "Sets your game mode to survival!", cmd_gms,
                     info.usages = {"/gms [player: player]"};
                     info.permissions = {"primebds.command.gms"};);

    /// Sets your game mode to survival!
    static bool cmd_gms(PrimeBDS &plugin, endstone::CommandSender &sender,
                        const std::vector<std::string> &args) {
        if (args.empty()) {
            auto *player = sender.asPlayer();
            if (!player) {
                sender.sendMessage("This command can only be executed by a player");
                return false;
            }
            player->setGameMode(endstone::GameMode::Survival);
            player->sendMessage("Set own game mode to Survival");
            return true;
        }
        auto targets = utils::getMatchingActors(plugin.getServer(), args[0], sender);
        for (auto *t : targets) {
            if (auto *p = dynamic_cast<endstone::Player *>(t)) {
                p->setGameMode(endstone::GameMode::Survival);
                p->sendMessage("Your game mode has been updated to Survival");
            }
        }
        sender.sendMessage("\u00a7e" + std::to_string(targets.size()) + " \u00a7rplayers were set to Survival");
        return true;
    }

} // namespace primebds::commands
