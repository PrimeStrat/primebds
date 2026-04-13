from endstone import Player, GameMode
from endstone.command import CommandSender
from endstone_primebds.utils.command_util import create_command
from endstone_primebds.utils.target_selector_util import get_matching_actors

from typing import TYPE_CHECKING
if TYPE_CHECKING:
    from endstone_primebds.primebds import PrimeBDS

# Register command
command, permission = create_command(
    "gma",
    "Sets your game mode to adventure!",
    ["/gma [player: player]"],
    ["primebds.command.gma"]
)

# GMA COMMAND FUNCTIONALITY
def handler(self: "PrimeBDS", sender: CommandSender, args: list[str]) -> bool:
    if len(args) == 0:
        if not isinstance(sender, Player):
            sender.send_message("This command can only be executed by a player")
            return False
        sender.game_mode = GameMode.ADVENTURE
        sender.send_message("Set own game mode to Adventure")
        return True

    targets = get_matching_actors(self, args[0], sender)
    for target in targets:
        target.game_mode = GameMode.ADVENTURE
        target.send_message("Your game mode has been updated to Adventure")
    sender.send_message(f"§e{len(targets)} §rplayers were set to Adventure")

    return True
