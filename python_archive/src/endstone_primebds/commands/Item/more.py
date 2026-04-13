from endstone import Player
from endstone.command import CommandSender
from endstone_primebds.utils.command_util import create_command
from endstone_primebds.utils.target_selector_util import get_matching_actors

from typing import TYPE_CHECKING
if TYPE_CHECKING:
    from endstone_primebds.primebds import PrimeBDS

# Register command
command, permission = create_command(
    "more",
    "Sets a full stack to the held item!",
    ["/more"],
    ["primebds.command.more"]
)

# MORE COMMAND FUNCTIONALITY
def handler(self: "PrimeBDS", sender: CommandSender, args: list[str]) -> bool:
    if len(args) == 0:
        if not isinstance(sender, Player):
            sender.send_message("§cThis command can only be executed by a player")
            return False

        held_item = sender.inventory.item_in_main_hand
        if held_item is None:
            sender.send_message("§cYou are not holding an item to stack")
            return False

        held_item.amount = held_item.max_stack_size
        sender.inventory.set_item(sender.inventory.held_item_slot, held_item)

        sender.send_message("§aYour held item is now a full stack")
        return True

    return True
