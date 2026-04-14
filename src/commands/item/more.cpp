#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"

namespace primebds::commands
{

    static bool cmd_more(PrimeBDS &plugin, endstone::CommandSender &sender,
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
            sender.sendMessage("\u00a7cYou are not holding an item to stack");
            return false;
        }

        held->setAmount(held->getMaxStackSize());
        player->getInventory().setItem(player->getInventory().getHeldItemSlot(), *held);
        player->sendMessage("\u00a7aYour held item is now a full stack");
        return true;
    }

    REGISTER_COMMAND(more, "Sets a full stack to the held item!", cmd_more,
                     info.usages = {"/more"};
                     info.permissions = {"primebds.command.more"};);
} // namespace primebds::commands
