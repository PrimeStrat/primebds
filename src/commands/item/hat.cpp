#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"
#include "primebds/utils/target_selector.h"

namespace primebds::commands
{

    static bool cmd_hat(PrimeBDS &plugin, endstone::CommandSender &sender,
                        const std::vector<std::string> &args)
    {
        auto *player = sender.asPlayer();
        if (!player)
        {
            sender.sendMessage("\u00a7cThis command can only be executed by a player");
            return false;
        }

        auto held = player->getInventory().getItemInMainHand();
        if (!held || held->getType() == endstone::ItemType::Air)
        {
            sender.sendMessage("\u00a7cYou are not holding an item");
            return false;
        }

        if (args.empty())
        {
            player->getInventory().setHelmet(*held);
            player->getInventory().clear(player->getInventory().getHeldItemSlot());
            player->sendMessage("\u00a7eYour " + std::string(held->getType().getId()) + " is now a hat!");
            return true;
        }

        if (!player->hasPermission("primebds.command.hat.other"))
        {
            player->sendMessage("\u00a7cYou do not have permission to hat another's held item");
            return true;
        }

        auto targets = utils::getMatchingActors(plugin.getServer(), args[0], sender);
        for (auto *t : targets)
        {
            auto *p = dynamic_cast<endstone::Player *>(t);
            if (p)
                p->getInventory().setHelmet(*held);
        }
        sender.sendMessage("\u00a7e" + std::to_string(targets.size()) + " \u00a7rplayers are now wearing " + std::string(held->getType().getId()));
        return true;
    }

    REGISTER_COMMAND(hat, "Sets the item in-hand as a hat!", cmd_hat,
                     info.usages = {"/hat [player: player]"};
                     info.permissions = {"primebds.command.hat", "primebds.command.hat.other"};);
} // namespace primebds::commands
