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
    "unmute",
    "Removes an active mute from a player!",
    ["/unmute <player: player>"],
    ["primebds.command.unmute"]
)

# UNMUTE COMMAND FUNCTIONALITY
def handler(self: "PrimeBDS", sender: CommandSender, args: list[str]) -> bool:
    if BlockCommandSender is not None and isinstance(sender, BlockCommandSender):
       sender.send_message("§cThis command cannot be automated")
       return False

    if len(args) < 1:
        sender.send_message(f"Usage: /unmute <player>")
        return False
    
    if any("@" in arg for arg in args):
        sender.send_message(f"§cTarget selectors are invalid for this command")
        return False

    player_name = args[0].strip('"')
    
    mod_log = self.db.get_offline_mod_log(player_name)

    if not mod_log or not mod_log.is_muted:
        sender.send_message(f"§6Player §e{player_name} §6is not muted")
        
        return False

    self.db.remove_mute(player_name)
    sender.send_message(f"§6Player §e{player_name} §6has been unmuted")
    log(self, f"§6Player §e{player_name} §6was unmuted by §e{sender.name}", "mod")
    
    user = self.server.get_player(player_name)
    if user is not None:
        user.send_message(f"§6Your mute has expired!")

    return True
