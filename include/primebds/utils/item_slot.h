/// @file item_slot.h
/// Utility for resolving inventory slot types in item commands.

#pragma once

#include <endstone/inventory/player_inventory.h>
#include <endstone/inventory/item_stack.h>

#include <optional>
#include <string>
#include <algorithm>
#include <vector>

namespace primebds::utils {

    struct SlotInfo {
        std::string type;
        int index = -1;
    };

    /// Parse optional slotType and slot index from args at the given offset.
    inline SlotInfo parseSlotArgs(const std::vector<std::string> &args, size_t offset) {
        SlotInfo info;
        if (offset < args.size()) {
            info.type = args[offset];
            std::transform(info.type.begin(), info.type.end(), info.type.begin(), ::tolower);
        }
        if (offset + 1 < args.size()) {
            try {
                info.index = std::stoi(args[offset + 1]);
            } catch (...) {}
        }
        return info;
    }

    /// Get an item from a player's inventory based on slot type.
    inline std::optional<endstone::ItemStack> getItemFromSlot(endstone::PlayerInventory &inv,
                                                               const SlotInfo &slot) {
        if (slot.type == "helmet") return inv.getHelmet();
        if (slot.type == "chestplate") return inv.getChestplate();
        if (slot.type == "leggings") return inv.getLeggings();
        if (slot.type == "boots") return inv.getBoots();
        if (slot.type == "offhand") return inv.getItemInOffHand();
        if (slot.type == "mainhand") return inv.getItemInMainHand();
        if (slot.type == "slot" && slot.index >= 0) return inv.getItem(slot.index);
        return inv.getItemInMainHand();
    }

    /// Put an item back into a player's inventory at the correct slot.
    inline void setItemToSlot(endstone::PlayerInventory &inv, const SlotInfo &slot,
                              const endstone::ItemStack &item) {
        if (slot.type == "helmet") { inv.setHelmet(item); return; }
        if (slot.type == "chestplate") { inv.setChestplate(item); return; }
        if (slot.type == "leggings") { inv.setLeggings(item); return; }
        if (slot.type == "boots") { inv.setBoots(item); return; }
        if (slot.type == "offhand") { inv.setItemInOffHand(item); return; }
        if (slot.type == "slot" && slot.index >= 0) { inv.setItem(slot.index, item); return; }
        inv.setItem(inv.getHeldItemSlot(), item);
    }

    /// Check if the slot type string is valid.
    inline bool isValidSlotType(const std::string &type) {
        return type.empty() || type == "helmet" || type == "chestplate" ||
               type == "leggings" || type == "boots" || type == "mainhand" ||
               type == "offhand" || type == "slot";
    }

} // namespace primebds::utils
