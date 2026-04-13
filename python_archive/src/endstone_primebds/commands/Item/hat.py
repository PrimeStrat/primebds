from endstone import Player
from endstone.command import CommandSender
from endstone_primebds.utils.command_util import create_command
from endstone_primebds.utils.target_selector_util import get_matching_actors

from typing import TYPE_CHECKING
if TYPE_CHECKING:
    from endstone_primebds.primebds import PrimeBDS

# Register command
command, permission = create_command(
    "hat",
    "Sets the item in-hand as a hat!",
    ["/hat [player: player]"],
    ["primebds.command.hat", "primebds.command.hat.other"]
)

# REPAIR COMMAND FUNCTIONALITY
def handler(self: "PrimeBDS", sender: CommandSender, args: list[str]) -> bool:

    if not isinstance(sender, Player):
        sender.send_message("§cThis command can only be executed by a player")
        return False

    held_item = sender.inventory.item_in_main_hand
    if held_item is None:
        sender.send_message("§cYou are not holding an item to repair")
        return False

    if len(args) == 0:
        sender.inventory.helmet = held_item
        sender.inventory.clear(sender.inventory.held_item_slot)
        sender.send_message(f"§eYour {held_item.type.id} is now a hat!")
        return True

    if not sender.has_permission("primebds.command.repair.other"):
        sender.send_message("§cYou do not have permission to hat another's held item")
        return True

    targets = get_matching_actors(self, args[0], sender)
    if not targets:
        sender.send_message("§cNo matching players found")
        return False

    hat_count = 0
    for target in targets:
        target.inventory.helmet = held_item
        hat_count += 1

    if hat_count == 1:
        sender.send_message(f"§eYour {held_item.type.id} is now a hat for {target.name}!")
    else:
        sender.send_message(f"§e{hat_count} §rplayers' is now wearing {held_item.type.id}")

    return True
