from endstone import Player, GameMode
from endstone.command import CommandSender
from endstone_primebds.utils.command_util import create_command
from endstone_primebds.utils.target_selector_util import get_matching_actors

from typing import TYPE_CHECKING
if TYPE_CHECKING:
    from endstone_primebds.primebds import PrimeBDS

# Register command
command, permission = create_command(
    "gmsp",
    "Sets your game mode to spectator!",
    ["/gmsp [player: player]"],
    ["primebds.command.gmsp"]
)

# GMSP COMMAND FUNCTIONALITY
def handler(self: "PrimeBDS", sender: CommandSender, args: list[str]) -> bool:
    if len(args) == 0:
        if not isinstance(sender, Player):
            sender.send_message("This command can only be executed by a player")
            return False
        sender.game_mode = GameMode.SPECTATOR
        sender.send_message("Set own game mode to Spectator")
        return True

    targets = get_matching_actors(self, args[0], sender)
    for target in targets:
        target.game_mode = GameMode.SPECTATOR
        target.send_message("Your game mode has been updated to Spectator")
    sender.send_message(f"§e{len(targets)} §rplayers were set to Spectator")

    return True
