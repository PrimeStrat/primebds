from endstone import Player
from endstone.command import CommandSender
from endstone_primebds.utils.commandUtil import create_command
from endstone_primebds.utils.targetSelectorUtil import get_matching_actors

from typing import TYPE_CHECKING
if TYPE_CHECKING:
    from endstone_primebds.primebds import PrimeBDS

# Register command
command, permission = create_command(
    "heal",
    "Sets player health to full!",
    ["/heal [player: player]"],
    ["primebds.command.heal"]
)

# GMA COMMAND FUNCTIONALITY
def handler(self: "PrimeBDS", sender: CommandSender, args: list[str]) -> bool:
    if len(args) == 0:
        if not isinstance(sender, Player):
            sender.send_message("This command can only be executed by a player")
            return False
        sender.health = sender.max_health
        sender.send_message(f'§aYou were healed')
        return True

    targets = get_matching_actors(self, args[0], sender)
    for target in targets:
        target.health = target.max_health
        target.send_message('§aYou were healed')
        
    if len(targets) == 1:
        sender.send_message(f'§e{targets[0].name} §rwas healed')
    else:
        sender.send_message(f'§e{len(targets)} §rplayers were healed')

    return True
