from endstone.command import CommandSender
from endstone_primebds.utils.command_util import create_command
from endstone_primebds.utils.target_selector_util import get_matching_actors

from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from endstone_primebds.primebds import PrimeBDS

# Register command
command, permission = create_command(
    "popup",
    "Sends a custom popup message!",
    ["/popup <player: player> <text: message>"],
    ["primebds.command.popup"]
)

# POPUP COMMAND FUNCTIONALITY
def handler(self: "PrimeBDS", sender: CommandSender, args: list[str]) -> bool:

    if len(args) < 2:
        sender.send_message("§cUsage: /popup <player> <text>")
        return False

    targets = get_matching_actors(self, args[0], sender)
    if not targets:
        sender.send_message(f"§cNo players found matching '{args[0]}'")
        return False

    message = " ".join(args[1:]).strip() if len(args) > 1 else ""
    if not message:
        sender.send_message("§cPopup message cannot be empty")
        return False

    for target in targets:
        target.send_popup(message)

    if len(targets) == 1:
        sender.send_message(f"§aPopup sent to §e{targets[0].name}")
    else:
        sender.send_message(f"§aPopup sent to §e{len(targets)} players")

    return True
