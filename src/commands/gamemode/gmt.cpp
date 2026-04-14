#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"
#include "primebds/utils/target_selector.h"

namespace primebds::commands
{

    static bool cmd_gmt(PrimeBDS &plugin, endstone::CommandSender &sender,
                        const std::vector<std::string> &args)
    {
        if (args.empty())
        {
            auto *player = sender.asPlayer();
            if (!player)
            {
                sender.sendMessage("This command can only be executed by a player");
                return false;
            }
            if (player->getGameMode() == endstone::GameMode::Creative)
            {
                player->setGameMode(endstone::GameMode::Survival);
                player->sendMessage("Set own game mode to Survival");
            }
            else
            {
                player->setGameMode(endstone::GameMode::Creative);
                player->sendMessage("Set own game mode to Creative");
            }
            return true;
        }
        auto targets = utils::getMatchingActors(plugin.getServer(), args[0], sender);
        for (auto *t : targets)
        {
            if (auto *p = dynamic_cast<endstone::Player *>(t))
            {
                if (p->getGameMode() == endstone::GameMode::Creative)
                {
                    p->setGameMode(endstone::GameMode::Survival);
                    p->sendMessage("Your game mode has been updated to Survival");
                }
                else
                {
                    p->setGameMode(endstone::GameMode::Creative);
                    p->sendMessage("Your game mode has been updated to Creative");
                }
            }
        }
        sender.sendMessage("\u00a7e" + std::to_string(targets.size()) + " \u00a7rplayers had their gamemode toggled");
        return true;
    }

    REGISTER_COMMAND(gmt, "Toggles you between survival and creative mode!", cmd_gmt,
                     info.usages = {"/gmt [player: player]"};
                     info.permissions = {"primebds.command.gmt"};);
} // namespace primebds::commands
