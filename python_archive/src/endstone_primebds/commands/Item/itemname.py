from endstone.command import CommandSender
from endstone_primebds.utils.command_util import create_command
from endstone_primebds.utils.target_selector_util import get_matching_actors

from typing import TYPE_CHECKING
if TYPE_CHECKING:
    from endstone_primebds.primebds import PrimeBDS

# Register command
command, permission = create_command(
    "itemname",
    "Modify item name data!",
    [
        "/itemname <player: player> (set)<set_name: set_name> <item_name: string> (slot|helmet|chestplate|leggings|boots|mainhand|offhand)[slotType: slotTypeSetName] [slot: int]",
        "/itemname <player: player> (clear)<clear_name: clear_name> (slot|helmet|chestplate|leggings|boots|mainhand|offhand)[slotType: slotTypeClearName] [slot: int]"
    ],
    ["primebds.command.itemname"]
)

def handler(self: "PrimeBDS", sender: CommandSender, args: list[str]) -> bool:
    if len(args) < 2:
        sender.send_message("§cUsage: /itemname <player> <set|clear> [name] [slotType] [slot]")
        return False

    target_selector = args[0]
    action = args[1].lower()
    name = args[2] if len(args) > 2 else None

    targets = get_matching_actors(self, target_selector, sender)
    if not targets:
        sender.send_message("§cNo matching players found")
        return False

    slot_type = None
    slot_index = None
    if len(args) > 3:
        slot_type = args[3].lower()
    if len(args) > 4:
        try:
            slot_index = int(args[4])
        except ValueError:
            sender.send_message("§cSlot index must be a number")
            return False

    for target in targets:
        # pick the item based on slotType or slot number
        held_item = None
        if slot_type:
            inv = target.inventory
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
            held_item = target.inventory.get_item(slot_index)
        else:
            held_item = target.inventory.item_in_main_hand

        if held_item is None:
            continue

        meta_data = held_item.item_meta
        if action == "set":
            if not name:
                sender.send_message("§cPlease specify a name to set")
                return False
            meta_data.display_name = name
        elif action == "clear":
            meta_data.display_name = None
            meta_data.lore = None
        else:
            sender.send_message("§cInvalid action. Use 'set' or 'clear'")
            return False

        held_item.set_item_meta(meta_data)

        # Put item back in correct slot
        if slot_type == "slot" and slot_index is not None:
            target.inventory.set_item(slot_index, held_item)
        elif slot_type == "helmet":
            target.inventory.helmet = held_item
        elif slot_type == "chestplate":
            target.inventory.chestplate = held_item
        elif slot_type == "leggings":
            target.inventory.leggings = held_item
        elif slot_type == "boots":
            target.inventory.boots = held_item
        elif slot_type == "offhand":
            target.inventory.item_in_off_hand = held_item
        else:
            target.inventory.set_item(target.inventory.held_item_slot, held_item)

    if len(targets) == 1:
        sender.send_message(f"§e{targets[0].name}§r's item name was updated")
    else:
        sender.send_message(f"§e{len(targets)} §rplayers' item names were updated")

    return True
