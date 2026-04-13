from endstone.command import CommandSender
from endstone_primebds.utils.command_util import create_command
from endstone_primebds.utils.target_selector_util import get_matching_actors

from typing import TYPE_CHECKING
if TYPE_CHECKING:
    from endstone_primebds.primebds import PrimeBDS

# Register command
command, permission = create_command(
    "itemlore",
    "Modify item lore data!",
    [
        "/itemlore <player: player> (add)<add_lore: add_lore> <item_lore: string> (slot|helmet|chestplate|leggings|boots|mainhand|offhand)[slotType: slotTypeAddLore] [slot: int]",
        "/itemlore <player: player> (set)<set_lore: set_lore> <replaced_lore: string> (slot|helmet|chestplate|leggings|boots|mainhand|offhand)[slotType: slotTypeSetLore] [slot: int]",
        "/itemlore <player: player> (delete)<delete_lore: delete_lore> [item_lore_line: int] (slot|helmet|chestplate|leggings|boots|mainhand|offhand)[slotType: slotTypeDeleteLore] [slot: int]",
        "/itemlore <player: player> (clear)<clear_lore: clear_lore> (slot|helmet|chestplate|leggings|boots|mainhand|offhand)[slotType: slotTypeSlotClearLore] [slot: int]"
    ],
    ["primebds.command.itemlore"],
    "op",
    ["lore"]
)


def handler(self: "PrimeBDS", sender: CommandSender, args: list[str]) -> bool:
    if len(args) < 2:
        sender.send_message("§cUsage: /itemlore <player> <add|set|delete|clear> [message|line] [slotType] [slot]")
        return False

    target_selector = args[0]
    action = args[1].lower()
    extra = args[2] if len(args) > 2 else None

    # optional slotType + slot
    slot_type = args[3].lower() if len(args) > 3 else None
    slot_index = None
    if len(args) > 4:
        try:
            slot_index = int(args[4])
        except ValueError:
            sender.send_message("§cSlot index must be a number")
            return False

    targets = get_matching_actors(self, target_selector, sender)
    if not targets:
        sender.send_message("§cNo matching players found")
        return False

    for target in targets:
        inv = target.inventory

        # pick the item based on slotType or slot number
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
            continue

        meta_data = held_item.item_meta
        current_lore = meta_data.lore or []

        if action == "add":
            if not extra:
                sender.send_message("§cPlease specify a lore line to add")
                return False
            new_lore = list(current_lore)
            new_lore.append(" ".join(args[2:len(args) if not slot_type else 3]))  # keep full text if before slotType
            meta_data.lore = new_lore

        elif action == "set":
            if not extra:
                sender.send_message("§cPlease specify lore text to set")
                return False
            meta_data.lore = [" ".join(args[2:len(args) if not slot_type else 3])]

        elif action == "delete":
            if not current_lore:
                sender.send_message("§cNo lore lines to delete")
                return False
            if extra and extra.isdigit():
                line_index = int(extra) - 1
                if 0 <= line_index < len(current_lore):
                    new_lore = list(current_lore)
                    new_lore.pop(line_index)
                    meta_data.lore = new_lore
                else:
                    sender.send_message("§cInvalid lore line index")
                    return False
            else:
                new_lore = list(current_lore)
                new_lore.pop()
                meta_data.lore = new_lore

        elif action == "clear":
            meta_data.lore = None

        else:
            sender.send_message("§cInvalid action. Use add, set, delete, or clear")
            return False

        held_item.set_item_meta(meta_data)

        if slot_type == "slot" and slot_index is not None:
            inv.set_item(slot_index, held_item)
        elif slot_type == "helmet":
            inv.helmet = held_item
        elif slot_type == "chestplate":
            inv.chestplate = held_item
        elif slot_type == "leggings":
            inv.leggings = held_item
        elif slot_type == "boots":
            inv.boots = held_item
        elif slot_type == "offhand":
            inv.item_in_off_hand = held_item
        else:
            inv.set_item(inv.held_item_slot, held_item)

    if len(targets) == 1:
        sender.send_message(f"§e{targets[0].name}§r's item lore was updated")
    else:
        sender.send_message(f"§e{len(targets)} §rplayers' item lores were updated")

    return True
