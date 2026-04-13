from endstone import Player
from endstone.command import CommandSender
from endstone_primebds.utils.command_util import create_command
from endstone_primebds.utils.target_selector_util import get_matching_actors

from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from endstone_primebds.primebds import PrimeBDS

# Register command for fly state
command, permission = create_command(
    "fly",
    "Enable or disable flying for a player!",
    ["/fly [player: player] [flight: bool]"],
    ["primebds.command.fly"]
)

def handler(self: "PrimeBDS", sender: CommandSender, args: list[str]) -> bool:

    if not isinstance(sender, Player):
        sender.send_error_message(f"This command can only be executed by a player")
        return False

    if len(args) == 0:
        if not isinstance(sender, Player):
            sender.send_message("You must specify a player when running this command from console")
            return False
        player = sender
        new_fly_state = not player.allow_flight
        player.allow_flight = new_fly_state
        msg = "§aFly Enabled§r" if new_fly_state else "§cFly Disabled§r"
        player.send_message(msg)
        return True

    if len(args) < 2:
        sender.send_message("Usage: /fly <player> <true|false>")
        return False

    targets = get_matching_actors(self, args[0], sender)
    if not targets:
        sender.send_message(f"Player(s) {args[0]} not found!")
        return False

    value_str = args[1].lower()
    if value_str not in ("true", "false"):
        sender.send_message(f"Invalid fly state value: {args[1]} (must be true or false)")
        return False

    new_fly_state = (value_str == "true")

    for player in targets:
        player.allow_flight = new_fly_state
        msg = "§aFly Enabled§r" if new_fly_state else "§cFly Disabled§r"
        player.send_message(msg)

    sender.send_message(f"§e{len(targets)} players §rfly state changed: {msg}")

    return True
