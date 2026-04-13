from endstone.command import CommandSender
try:
    from endstone.command import BlockCommandSender
except ImportError:
    BlockCommandSender = None 
from endstone_primebds.utils.command_util import create_command

from endstone_primebds.utils.logging_util import log

from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from endstone_primebds.primebds import PrimeBDS

# Register command
command, permission = create_command(
    "unnameban",
    "Removes an active name ban from a player!",
    ["/unnameban <player: player>"],
    ["primebds.command.unnameban"]
)

# REMOVEBAN COMMAND FUNCTIONALITY
def handler(self: "PrimeBDS", sender: CommandSender, args: list[str]) -> bool:
    if BlockCommandSender is not None and isinstance(sender, BlockCommandSender):
       sender.send_message("§cThis command cannot be automated")
       return False

    if len(args) < 1:
        sender.send_message(f"Usage: /removeban <player>")
        return False
    
    if any("@" in arg for arg in args):
        sender.send_message(f"§cTarget selectors are invalid for this command")
        return False

    player_name = args[0]

    if not self.serverdb.check_nameban(player_name):
        sender.send_message(f"§6Player §e{player_name} §6is not name banned")
        return False

    self.serverdb.remove_name(player_name)
    sender.send_message(f"§6Player §e{player_name}'s §6name is no longer banned")

    log(self, f"§6Player §e{player_name} §6is no longer name banned by §e{sender.name}", "mod")

    return True

