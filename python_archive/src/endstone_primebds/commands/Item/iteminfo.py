from endstone.command import CommandSender
from endstone_primebds.utils.command_util import create_command
from endstone_primebds.utils.target_selector_util import get_matching_actors

from typing import TYPE_CHECKING
if TYPE_CHECKING:
    from endstone_primebds.primebds import PrimeBDS

# Register command
command, permission = create_command(
    "iteminfo",
    "Check item data!",
    [
        "/iteminfo [player: player] (slot|helmet|chestplate|leggings|boots|mainhand|offhand)[slotTypeInfo: slotTypeInfo] [slot: int]"
    ],
    ["primebds.command.iteminfo"]
)

def handler(self: "PrimeBDS", sender: CommandSender, args: list[str]) -> bool:
    slot_type = None
    slot_index = None
    targets = []

    if len(args) >= 1:
        possible_target = args[0].lower()
        valid_slots = {"helmet", "chestplate", "leggings", "boots", "mainhand", "offhand", "slot"}
        if possible_target in valid_slots:
            slot_type = possible_target
            if len(args) > 1:
                try:
                    slot_index = int(args[1])
                except ValueError:
                    sender.send_message("§cSlot index must be a number")
                    return False
            targets = [sender]
        else:
            target_selector = args[0]
            targets = get_matching_actors(self, target_selector, sender)

            if not targets:
                sender.send_message("§cNo matching players found")
                return False

            if len(args) > 1:
                slot_type = args[1].lower()
            if len(args) > 2:
                try:
                    slot_index = int(args[2])
                except ValueError:
                    sender.send_message("§cSlot index must be a number")
                    return False
    else:
        targets = [sender]

    for target in targets:
        inv = target.inventory

        held_item = None
        if slot_type:
            if slot_type == "helmet":
                held_item = inv.helmet
            elif slot_type == "chestplate":
                held_item = inv.chestplate
            elif slot_type == "leggings":
                held_item = inv.leggings
            elif slot_type == "boots":
                held_item = inv.boots
            elif slot_type == "offhand":
                held_item = inv.item_in_off_hand
            elif slot_type == "mainhand":
                held_item = inv.item_in_main_hand
            elif slot_type == "slot" and slot_index is not None:
                held_item = inv.get_item(slot_index)
            else:
                sender.send_message(f"§cUnknown slot type '{slot_type}'")
                return False
        elif slot_index is not None:
            held_item = inv.get_item(slot_index)
        else:
            held_item = inv.item_in_main_hand

        if held_item is None:
            sender.send_message(f"§c{target.name} does not have an item in the specified slot")
            continue

        meta_data = held_item.item_meta

        lines = []
        lines.append(f"§bItem Info for {target.name}:")
        lines.append(f"§7- §eType: §f{held_item.type}")
        data_value = getattr(held_item, "data", None)
        lines.append(f"§7- §eData: §f{data_value if data_value is not None else '?'}")
        lines.append(f"§7- §eAmount: §f{held_item.amount}")

        if slot_type:
            slot_string = f"§7- §eSlot: §f{slot_type}"
            if slot_index is not None:
                lines.append(f"{slot_string} §8[§b{slot_index}§8]")
            else:
                lines.append(slot_string)
        else:
            lines.append("§7- §eSlot: §fMainhand")

        if meta_data:
            if meta_data.display_name:
                lines.append(f"§7- §eDisplay Name: §r{meta_data.display_name}")

            if meta_data.damage:
                lines.append(f"§7- §eDamage Taken: §r{meta_data.damage}")

            lines.append(f"§7- §eUnbreakable: §r{meta_data.is_unbreakable}")

            if meta_data.lore:
                lines.append("§7- §eLore:")
                for i, lore_line in enumerate(meta_data.lore, start=1):
                    lines.append(f"  §7- §7{i}. §b{lore_line}")

            if meta_data.has_enchants:
                lines.append("§7- §eEnchantments:")
                for ench, lvl in meta_data.enchants.items():
                    lines.append(f"  §7- §d{ench} §r{lvl}")

        sender.send_message("\n".join(lines))

    return True
