#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"
#include "primebds/utils/target_selector.h"

namespace primebds::commands
{

    static bool cmd_gmc(PrimeBDS &plugin, endstone::CommandSender &sender,
                        const std::vector<std::string> &args)
    {
        if (args.empty())
        {
            auto *player = dynamic_cast<endstone::Player *>(&sender);
            if (!player)
            {
                sender.sendMessage("This command can only be executed by a player");
                return false;
            }
            player->setGameMode(endstone::GameMode::Creative);
            player->sendMessage("Set own game mode to Creative");
            return true;
        }
        auto targets = utils::getMatchingActors(plugin.getServer(), args[0], sender);
        for (auto *t : targets)
        {
            if (auto *p = dynamic_cast<endstone::Player *>(t))
            {
                p->setGameMode(endstone::GameMode::Creative);
                p->sendMessage("Your game mode has been updated to Creative");
            }
        }
        sender.sendMessage("\u00a7e" + std::to_string(targets.size()) + " \u00a7rplayers were set to Creative");
        return true;
    }

    REGISTER_COMMAND(gmc, "Sets your game mode to creative!", cmd_gmc,
                     info.usages = {"/gmc [player: player]"};
                     info.permissions = {"primebds.command.gmc"};);
} // namespace primebds::commands
