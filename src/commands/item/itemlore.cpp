/// @file itemlore.cpp
/// Modify item lore data!

#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"
#include "primebds/utils/item_slot.h"
#include "primebds/utils/target_selector.h"

#include <algorithm>
#include <string>

namespace primebds::commands {

    static bool cmd_itemlore(PrimeBDS &, endstone::CommandSender &,
                        const std::vector<std::string> &);

    REGISTER_COMMAND(itemlore, "Modify item lore data!", cmd_itemlore,
                     info.usages = {
                         "/itemlore <player: player> (add)<action: lore_action> <item_lore: string> (slot|helmet|chestplate|leggings|boots|mainhand|offhand)[slotType: slotType] [slot: int]",
                         "/itemlore <player: player> (set)<action: lore_action> <replaced_lore: string> (slot|helmet|chestplate|leggings|boots|mainhand|offhand)[slotType: slotType] [slot: int]",
                         "/itemlore <player: player> (delete)<action: lore_action> [item_lore_line: int] (slot|helmet|chestplate|leggings|boots|mainhand|offhand)[slotType: slotType] [slot: int]",
                         "/itemlore <player: player> (clear)<action: lore_action> (slot|helmet|chestplate|leggings|boots|mainhand|offhand)[slotType: slotType] [slot: int]"};
                     info.permissions = {"primebds.command.itemlore"};
                     info.default_permission = "op";
                     info.aliases = {"lore"};);

    /// Modify item lore data!
    static bool cmd_itemlore(PrimeBDS &plugin, endstone::CommandSender &sender,
                             const std::vector<std::string> &args) {
        if (args.size() < 2) {
            sender.sendMessage("\u00a7cUsage: /itemlore <player> <add|set|delete|clear> [message|line] [slotType] [slot]");
            return false;
        }
        auto targets = utils::getMatchingActors(plugin.getServer(), args[0], sender);
        if (targets.empty()) {
            sender.sendMessage("\u00a7cNo matching players found");
            return false;
        }

        std::string action = args[1];
        std::transform(action.begin(), action.end(), action.begin(), ::tolower);
        std::string extra = (args.size() > 2) ? args[2] : "";

        // Determine where slotType args start based on action
        size_t slot_offset = 3;
        if (action == "clear")
            slot_offset = 2;
        else if (action == "delete" && (extra.empty() || std::all_of(extra.begin(), extra.end(), ::isdigit)))
            slot_offset = extra.empty() ? 2 : 3;

        auto slot = utils::parseSlotArgs(args, slot_offset);
        if (!slot.type.empty() && !utils::isValidSlotType(slot.type)) {
            sender.sendMessage("\u00a7cUnknown slot type '" + slot.type + "'");
            return false;
        }

        for (auto *t : targets) {
            auto *p = dynamic_cast<endstone::Player *>(t);
            if (!p)
                continue;
            auto held = utils::getItemFromSlot(p->getInventory(), slot);
            if (!held)
                continue;
            auto meta = held->getItemMeta();
            auto lore = meta->getLore();

            if (action == "add" && !extra.empty()) {
                lore.push_back(extra);
                meta->setLore(lore);
            } else if (action == "set" && !extra.empty()) {
                meta->setLore(std::vector<std::string>{extra});
            } else if (action == "delete") {
                if (!lore.empty()) {
                    if (!extra.empty()) {
                        int idx = std::stoi(extra) - 1;
                        if (idx >= 0 && idx < static_cast<int>(lore.size()))
                            lore.erase(lore.begin() + idx);
                    } else {
                        lore.pop_back();
                    }
                    meta->setLore(lore);
                }
            } else if (action == "clear") {
                meta->setLore(std::nullopt);
            } else {
                sender.sendMessage("\u00a7cInvalid action. Use add, set, delete, or clear");
                return false;
            }

            held->setItemMeta(meta.get());
            utils::setItemToSlot(p->getInventory(), slot, *held);
        }

        if (targets.size() == 1)
            sender.sendMessage("\u00a7e" + targets[0]->getName() + "\u00a7r's item lore was updated");
        else
            sender.sendMessage("\u00a7e" + std::to_string(targets.size()) + " \u00a7rplayers' item lores were updated");
        return true;
    }

} // namespace primebds::commands
