#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"
#include "primebds/utils/target_selector.h"

namespace primebds::commands
{

    static bool cmd_heal(PrimeBDS &plugin, endstone::CommandSender &sender,
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
            player->setHealth(player->getMaxHealth());
            player->sendMessage("\u00a7aYou were healed");
            return true;
        }
        if (!sender.hasPermission("primebds.command.heal.other"))
        {
            sender.sendMessage("\u00a7cYou do not have permission to heal others");
            return true;
        }
        auto targets = utils::getMatchingActors(plugin.getServer(), args[0], sender);
        for (auto *t : targets)
        {
            auto *p = dynamic_cast<endstone::Player *>(t);
            if (p)
            {
                p->setHealth(p->getMaxHealth());
                p->sendMessage("\u00a7aYou were healed");
            }
        }
        sender.sendMessage("\u00a7e" + std::to_string(targets.size()) + " \u00a7rplayers were healed");
        return true;
    }

    REGISTER_COMMAND(heal, "Heals all health to full!", cmd_heal,
                     info.usages = {"/heal [player: player]"};
                     info.permissions = {"primebds.command.heal", "primebds.command.heal.other"};
                     info.default_permission = "op";);
} // namespace primebds::commands
