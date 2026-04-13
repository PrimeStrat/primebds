from endstone import Player
from endstone.command import CommandSender
from endstone_primebds.utils.command_util import create_command
from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from endstone_primebds.primebds import PrimeBDS

command, permission = create_command(
    "setback",
    "Sets the global cooldown and delay for /back!",
    ["/setback [delay: int] [cooldown: int]"],
    ["primebds.command.setback"]
)

def handler(self: "PrimeBDS", sender: CommandSender, args: list[str]) -> bool:
    if not sender.has_permission("primebds.command.setback"):
        sender.send_error_message("You do not have permission to use this command")
        return False

    try:
        delay = float(args[0]) if len(args) >= 1 else 0
        cooldown = float(args[1]) if len(args) >= 2 else 0
    except ValueError:
        sender.send_error_message("Invalid arguments. Usage: /setback <cooldown> <delay>")
        return False

    self.serverdb.set_last_warp_settings(cooldown=cooldown, delay=delay)
    sender.send_message(f"§a/back cooldown set to §e{cooldown}s §aand delay set to §e{delay}s")
    return True
