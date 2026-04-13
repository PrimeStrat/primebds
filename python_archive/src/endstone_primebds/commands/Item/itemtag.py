from endstone.command import CommandSender
from endstone_primebds.utils.command_util import create_command
from endstone_primebds.utils.target_selector_util import get_matching_actors

from typing import TYPE_CHECKING
if TYPE_CHECKING:
    from endstone_primebds.primebds import PrimeBDS

# Register command
command, permission = create_command(
    "itemtag",
    "Modify item tags!",
    [
        "/itemtag <player: player> (unbreakable)<itemTag: itemTag> <is: bool> (slot|helmet|chestplate|leggings|boots|mainhand|offhand)[slotTypeTag: slotTypeTag] [slot: int]"
    ],
    ["primebds.command.itemtag"]
)

def handler(self: "PrimeBDS", sender: CommandSender, args: list[str]) -> bool:
    target_selector = args[0]
    tag_type = args[1].lower()
    tag_value_str = args[2].lower()
    if tag_value_str in ("true", "1", "yes", "on"):
        tag_value = True
    elif tag_value_str in ("false", "0", "no", "off"):
        tag_value = False
    else:
        sender.send_message("§cInvalid tag value")
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

    targets = get_matching_actors(self, target_selector, sender)

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
            continue

        if tag_type == "unbreakable":
            meta_data = held_item.item_meta
            print(tag_value)
            meta_data.is_unbreakable = tag_value
            held_item.set_item_meta(meta_data)

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

            status = "§aTrue" if tag_value else "§cFalse"
            sender.send_message(f"§e{target.name}§r's item tag §eUnbreakable §rset to {status}")

    return True
