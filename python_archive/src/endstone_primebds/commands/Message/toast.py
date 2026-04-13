from endstone import Player, ColorFormat
from endstone.command import CommandSender
from endstone_primebds.utils.command_util import create_command
from endstone_primebds.utils.target_selector_util import get_matching_actors

from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from endstone_primebds.primebds import PrimeBDS

# Register command
command, permission = create_command(
    "toast",
    "Sends a custom toast message!",
    ["/toast <player: player> <title: string> <text: message>"],
    ["primebds.command.toast"]
)

# TOAST COMMAND FUNCTIONALITY
def handler(self: "PrimeBDS", sender: CommandSender, args: list[str]) -> bool:
    if len(args) < 3:
        sender.send_message("§cUsage: /toast <player> <title> <text>")
        return False

    targets = get_matching_actors(self, args[0], sender)
    if not targets:
        sender.send_message(f"§cNo players found matching '{args[0]}'")
        return False

    title = args[1]
    message = " ".join(args[2:]).strip()
    if not message:
        sender.send_message("§cToast message cannot be empty")
        return False

    for player in targets:
        player.send_toast(title, message)

    if len(targets) == 1:
        sender.send_message(f"§aToast sent to §e{targets[0].name}")
    else:
        sender.send_message(f"§aToast sent to §e{len(targets)} players")

    return True
