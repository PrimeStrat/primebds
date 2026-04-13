from endstone import Player
from endstone.command import CommandSender
from endstone_primebds.utils.command_util import create_command
from endstone_primebds.utils.target_selector_util import get_matching_actors

from typing import TYPE_CHECKING
if TYPE_CHECKING:
    from endstone_primebds.primebds import PrimeBDS

# Register command
command, permission = create_command(
    "repair",
    "Repairs the item in hand!",
    ["/repair [player: player]"],
    ["primebds.command.repair", "primebds.command.repair.other"]
)

# REPAIR COMMAND FUNCTIONALITY
def handler(self: "PrimeBDS", sender: CommandSender, args: list[str]) -> bool:
    if len(args) == 0:
        if not isinstance(sender, Player):
            sender.send_message("§cThis command can only be executed by a player")
            return False

        held_item = sender.inventory.item_in_main_hand
        if held_item is None:
            sender.send_message("§cYou are not holding an item to repair")
            return False

        meta = held_item.item_meta
        meta.damage = 0
        held_item.set_item_meta(meta)
        sender.inventory.set_item(sender.inventory.held_item_slot, held_item)

        sender.send_message("§aYour held item was repaired")
        return True

    if not sender.has_permission("primebds.command.repair.other"):
        sender.send_message("§cYou do not have permission to repair others' items")
        return True

    targets = get_matching_actors(self, args[0], sender)
    if not targets:
        sender.send_message("§cNo matching players found")
        return False

    repaired_count = 0
    for target in targets:
        held_item = target.inventory.item_in_main_hand
        if held_item is None:
            continue

        meta = held_item.item_meta
        meta.damage = 0
        held_item.set_item_meta(meta)
        target.inventory.set_item(target.inventory.held_item_slot, held_item)

        target.send_message("§aYour held item was repaired")
        repaired_count += 1

    if repaired_count == 0:
        sender.send_message("§cNo items to repair for the selected targets")
    elif repaired_count == 1:
        sender.send_message(f"§e{targets[0].name}§r's item was repaired")
    else:
        sender.send_message(f"§e{repaired_count} §rplayers' items were repaired")

    return True
