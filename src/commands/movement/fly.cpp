#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"
#include "primebds/utils/target_selector.h"

namespace primebds::commands
{

    static bool cmd_fly(PrimeBDS &plugin, endstone::CommandSender &sender,
                        const std::vector<std::string> &args)
    {
        if (args.empty())
        {
            auto *player = dynamic_cast<endstone::Player *>(&sender);
            if (!player)
            {
                sender.sendMessage("\u00a7cOnly players can use this command.");
                return true;
            }

            bool flying = player->getAllowFlight();
            player->setAllowFlight(!flying);
            player->sendMessage(flying ? "\u00a7cFlight disabled" : "\u00a7aFlight enabled");
            return true;
        }

        if (!sender.hasPermission("primebds.command.fly.other"))
        {
            sender.sendMessage("\u00a7cYou don't have permission to toggle flight for others");
            return false;
        }

        auto targets = utils::getMatchingActors(plugin.getServer(), args[0], sender);
        for (auto *t : targets)
        {
            if (auto *p = dynamic_cast<endstone::Player *>(t))
            {
                bool flying = p->getAllowFlight();
                p->setAllowFlight(!flying);
                p->sendMessage(flying ? "\u00a7cFlight disabled" : "\u00a7aFlight enabled");
            }
        }
        sender.sendMessage("\u00a7aToggled flight for " + std::to_string(targets.size()) + " player(s)");
        return true;
    }

    REGISTER_COMMAND(fly, "Toggles flight for a player!", cmd_fly,
                     info.usages = {"/fly [player: player]"};
                     info.permissions = {"primebds.command.fly"};);
} // namespace primebds::commands
