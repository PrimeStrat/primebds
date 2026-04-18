/// @file iteminfo.cpp
/// View item information!

#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"
#include "primebds/utils/item_slot.h"
#include "primebds/utils/target_selector.h"

#include <algorithm>
#include <string>

namespace primebds::commands {

    static bool cmd_iteminfo(PrimeBDS &, endstone::CommandSender &,
                        const std::vector<std::string> &);

    REGISTER_COMMAND(iteminfo, "View item information!", cmd_iteminfo,
                     info.usages = {
                         "/iteminfo [player: player] (slot|helmet|chestplate|leggings|boots|mainhand|offhand)[slotType: slotType] [slot: int]"};
                     info.permissions = {"primebds.command.iteminfo"};);

    /// View item information!
    static bool cmd_iteminfo(PrimeBDS &plugin, endstone::CommandSender &sender,
                             const std::vector<std::string> &args) {
        endstone::Player *target = nullptr;
        size_t slot_offset = 0;

        // First arg could be a player or a slot type
        if (!args.empty()) {
            if (utils::isValidSlotType(args[0])) {
                // No player specified, slot type is first arg — sender must be a player
                auto *sp = dynamic_cast<endstone::Player *>(&sender);
                if (!sp) {
                    sender.sendMessage("\u00a7cYou must specify a player from the console");
                    return false;
                }
                target = sp;
                slot_offset = 0;
            } else {
                // First arg is a player selector
                auto targets = utils::getMatchingActors(plugin.getServer(), args[0], sender);
                if (targets.empty()) {
                    sender.sendMessage("\u00a7cNo matching players found");
                    return false;
                }
                target = dynamic_cast<endstone::Player *>(targets[0]);
                slot_offset = 1;
            }
        } else {
            auto *sp = dynamic_cast<endstone::Player *>(&sender);
            if (!sp) {
                sender.sendMessage("\u00a7cYou must specify a player from the console");
                return false;
            }
            target = sp;
        }

        if (!target) {
            sender.sendMessage("\u00a7cTarget is not a player");
            return false;
        }

        auto slot = utils::parseSlotArgs(args, slot_offset);
        auto held = utils::getItemFromSlot(target->getInventory(), slot);
        if (!held) {
            sender.sendMessage("\u00a7cNo item found in the specified slot");
            return false;
        }

        auto meta = held->getItemMeta();
        std::string type_name = held->getType().getId();
        int amount = held->getAmount();

        sender.sendMessage("\u00a7e--- Item Info ---");
        sender.sendMessage("\u00a7eType: \u00a7f" + type_name);
        sender.sendMessage("\u00a7eAmount: \u00a7f" + std::to_string(amount));
        if (meta->hasDisplayName())
            sender.sendMessage("\u00a7eName: \u00a7f" + meta->getDisplayName());
        if (meta->hasLore()) {
            sender.sendMessage("\u00a7eLore:");
            auto lore = meta->getLore();
            for (size_t i = 0; i < lore.size(); i++)
                sender.sendMessage("  \u00a7f" + std::to_string(i + 1) + ". " + lore[i]);
        }
        sender.sendMessage("\u00a7eUnbreakable: \u00a7f" + std::string(meta->isUnbreakable() ? "true" : "false"));
        if (meta->hasDamage())
            sender.sendMessage("\u00a7eDamage: \u00a7f" + std::to_string(meta->getDamage()));
        if (meta->hasEnchants()) {
            sender.sendMessage("\u00a7eEnchantments:");
            for (auto &[ench, level] : meta->getEnchants())
                sender.sendMessage("  \u00a7f" + std::string(ench->getId()) + " " + std::to_string(level));
        }
        return true;
    }

} // namespace primebds::commands
