from endstone import Player, GameMode
from endstone.command import CommandSender
from endstone_primebds.utils.command_util import create_command
from endstone_primebds.utils.target_selector_util import get_matching_actors

from typing import TYPE_CHECKING
if TYPE_CHECKING:
    from endstone_primebds.primebds import PrimeBDS

# Register command
command, permission = create_command(
    "gmt",
    "Toggles you between survival and creative mode!",
    ["/gmt [player: player]"],
    ["primebds.command.gmt"]
)

# GMT COMMAND FUNCTIONALITY
def handler(self: "PrimeBDS", sender: CommandSender, args: list[str]) -> bool:
    if len(args) == 0:
        if not isinstance(sender, Player):
            sender.send_message("This command can only be executed by a player")
            return False
        if sender.game_mode == GameMode.CREATIVE:
            sender.game_mode = GameMode.SURVIVAL
            sender.send_message("Set own game mode to Survival")
        else:
            sender.game_mode = GameMode.CREATIVE
            sender.send_message("Set own game mode to Creative")
        return True

    targets = get_matching_actors(self, args[0], sender)
    for target in targets:
        if target.game_mode == GameMode.CREATIVE:
            target.game_mode = GameMode.SURVIVAL
            target.send_message("Your game mode has been updated to Survival")
        else:
            target.game_mode = GameMode.CREATIVE
            target.send_message("Your game mode has been updated to Creative")
    sender.send_message(f"§e{len(targets)} §rplayers had their gamemode toggled")

    return True
