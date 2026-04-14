/// @file itemname.cpp
/// Modify item name data!

#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"
#include "primebds/utils/target_selector.h"

#include <algorithm>
#include <string>

namespace primebds::commands {

    static bool cmd_itemname(PrimeBDS &, endstone::CommandSender &,
                        const std::vector<std::string> &);

    REGISTER_COMMAND(itemname, "Modify item name data!", cmd_itemname,
                     info.usages = {"/itemname <player: player> (set|clear) [name: string]"};
                     info.permissions = {"primebds.command.itemname"};);

    /// Modify item name data!
    static bool cmd_itemname(PrimeBDS &plugin, endstone::CommandSender &sender,
                             const std::vector<std::string> &args) {
        if (args.size() < 2) {
            sender.sendMessage("\u00a7cUsage: /itemname <player> <set|clear> [name] [slotType] [slot]");
            return false;
        }
        auto targets = utils::getMatchingActors(plugin.getServer(), args[0], sender);
        if (targets.empty()) {
            sender.sendMessage("\u00a7cNo matching players found");
            return false;
        }

        std::string action = args[1];
        std::transform(action.begin(), action.end(), action.begin(), ::tolower);
        std::string name = (args.size() > 2) ? args[2] : "";

        for (auto *t : targets) {
            auto *p = dynamic_cast<endstone::Player *>(t);
            if (!p)
                continue;
            auto held = p->getInventory().getItemInMainHand();
            if (!held || held->getType() == endstone::ItemType::Air)
                continue;
            auto meta = held->getItemMeta();
            if (action == "set" && !name.empty())
                meta->setDisplayName(name);
            else if (action == "clear") {
                meta->setDisplayName(std::nullopt);
                meta->setLore(std::nullopt);
            } else {
                sender.sendMessage("\u00a7cInvalid action. Use 'set' or 'clear'");
                return false;
            }
            held->setItemMeta(meta.get());
            p->getInventory().setItem(p->getInventory().getHeldItemSlot(), *held);
        }

        sender.sendMessage("\u00a7e" + std::to_string(targets.size()) + " \u00a7rplayers' item names were updated");
        return true;
    }

} // namespace primebds::commands
