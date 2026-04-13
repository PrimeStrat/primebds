from endstone import Player
from endstone.command import CommandSender
from endstone_primebds.utils.command_util import create_command
from endstone_primebds.utils.target_selector_util import get_matching_actors

from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from endstone_primebds.primebds import PrimeBDS

# Register command
command, permission = create_command(
    "tip",
    "Sends a custom tip message!",
    ["/tip <player: player> <text: message>"],
    ["primebds.command.tip"]
)

# TIP COMMAND FUNCTIONALITY
def handler(self: "PrimeBDS", sender: CommandSender, args: list[str]) -> bool:
    if len(args) < 2:
        sender.send_message("§cUsage: /tip <player> <text>")
        return False

    targets = get_matching_actors(self, args[0], sender)
    if not targets:
        sender.send_message(f"§cNo players found matching '{args[0]}'")
        return False

    if len(args[1]) < 1:
        sender.send_message("§cTip message cannot be empty")
        return False

    for player in targets:
        player.send_tip(f"{args[1]}")

    if len(targets) == 1:
        sender.send_message(f"§aTip sent to §e{targets[0].name}")
    else:
        sender.send_message(f"§aTip sent to §e{len(targets)} players")

    return True
