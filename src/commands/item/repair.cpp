/// @file repair.cpp
/// Repairs the item in hand!

#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"
#include "primebds/utils/target_selector.h"

namespace primebds::commands {

    static bool cmd_repair(PrimeBDS &, endstone::CommandSender &,
                        const std::vector<std::string> &);

    REGISTER_COMMAND(repair, "Repairs the item in hand!", cmd_repair,
                     info.usages = {"/repair [player: player]"};
                     info.permissions = {"primebds.command.repair", "primebds.command.repair.other"};);

    /// Repairs the item in hand!
    static bool cmd_repair(PrimeBDS &plugin, endstone::CommandSender &sender,
                           const std::vector<std::string> &args) {
        if (args.empty()) {
            auto *player = sender.asPlayer();
            if (!player) {
                sender.sendMessage("\u00a7cThis command can only be executed by a player");
                return false;
            }
            auto held = player->getInventory().getItemInMainHand();
            if (!held || held->getType() == endstone::ItemType::Air) {
                sender.sendMessage("\u00a7cYou are not holding an item to repair");
                return false;
            }
            auto meta = held->getItemMeta();
            meta->setDamage(0);
            held->setItemMeta(meta.get());
            player->getInventory().setItem(player->getInventory().getHeldItemSlot(), *held);
            player->sendMessage("\u00a7aYour held item was repaired");
            return true;
        }

        if (!sender.hasPermission("primebds.command.repair.other")) {
            sender.sendMessage("\u00a7cYou do not have permission to repair others' items");
            return true;
        }

        auto targets = utils::getMatchingActors(plugin.getServer(), args[0], sender);
        int count = 0;
        for (auto *t : targets) {
            auto *p = dynamic_cast<endstone::Player *>(t);
            if (!p)
                continue;
            auto held = p->getInventory().getItemInMainHand();
            if (!held || held->getType() == endstone::ItemType::Air)
                continue;
            auto meta = held->getItemMeta();
            meta->setDamage(0);
            held->setItemMeta(meta.get());
            p->getInventory().setItem(p->getInventory().getHeldItemSlot(), *held);
            p->sendMessage("\u00a7aYour held item was repaired");
            ++count;
        }

        if (count == 0)
            sender.sendMessage("\u00a7cNo items to repair for the selected targets");
        else if (count == 1)
            sender.sendMessage("\u00a7eItem was repaired");
        else
            sender.sendMessage("\u00a7e" + std::to_string(count) + " \u00a7rplayers' items were repaired");
        return true;
    }

} // namespace primebds::commands
