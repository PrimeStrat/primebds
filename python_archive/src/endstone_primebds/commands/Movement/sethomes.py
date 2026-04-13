from endstone import Player
from endstone.command import CommandSender
from endstone_primebds.utils.command_util import create_command
from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from endstone_primebds.primebds import PrimeBDS

command, permission = create_command(
    "sethomes",
    "Set global home settings (delay, cooldown, cost)!",
    ["/sethomes <delay: int> <cooldown: int> <cost: int>"],
    ["primebds.command.sethomes"]
)

def handler(self: "PrimeBDS", sender: CommandSender, args: list[str]) -> bool:
    if not sender.has_permission("primebds.command.sethomes"):
        sender.send_error_message("You do not have permission to run this command")
        return False

    try:
        delay = float(args[0]) if len(args) >= 1 else 0
        cooldown = float(args[1]) if len(args) >= 2 else 0
        cost = float(args[2]) if len(args) >= 3 else 0
    except ValueError:
        sender.send_error_message("Invalid numbers provided. Usage: /sethomes <delay> <cooldown> <cost>")
        return False

    self.serverdb.set_home_settings(delay=delay, cooldown=cooldown, cost=cost)
    sender.send_message(f"§aGlobal home settings updated:\n§7- §eDelay: {delay:.1f}s\n§7- §eCooldown: {cooldown:.1f}s\n§7- §eCost: {cost:.2f}")
    return True
