from endstone.command import CommandSender
try:
    from endstone.command import BlockCommandSender
except ImportError:
    BlockCommandSender = None 
from endstone_primebds.utils.command_util import create_command

from endstone_primebds.utils.logging_util import log
from endstone_primebds.utils.mod_util import format_time_remaining
from datetime import timedelta, datetime

from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from endstone_primebds.primebds import PrimeBDS

# Register command
command, permission = create_command(
    "mute",
    "Permanently mutes a player from the server!",
    ["/mute <player: player> [reason: message]"],
    ["primebds.command.mute"]
)

def handler(self: "PrimeBDS", sender: CommandSender, args: list[str]) -> bool:
    if BlockCommandSender is not None and isinstance(sender, BlockCommandSender):
       sender.send_message("§cThis command cannot be automated")
       return False

    if len(args) < 1:
        sender.send_message(f"Usage: /mute <player> [reason]")
        return False
    
    if any("@" in arg for arg in args):
        sender.send_message(f"§cTarget selectors are invalid for this command")
        return False

    player_name = args[0].strip('"')
    target = self.server.get_player(player_name)

    if target:
        self.db.check_and_update_mute(target.xuid, target.name)

    # Check if the player is muted already
    mod_log = self.db.get_offline_mod_log(player_name)
    if mod_log and mod_log.is_muted:
        formatted_expiration = format_time_remaining(mod_log.mute_time, True)
        sender.send_message(f"§6Player §e{player_name} §6is already muted for §e{mod_log.mute_reason}§6, the mute expires §e{formatted_expiration}")
        
        return False

    mute_duration = timedelta(days=365 * 200)
    mute_expiration = datetime.now() + mute_duration
    reason = " ".join(args[3:]) if len(args) > 3 else "Negative Behavior"

    formatted_expiration = format_time_remaining(int(mute_expiration.timestamp()), True)
    message = f"§6You are muted for §e{reason} §6which expires §e{formatted_expiration}"

    if target:
        # If the player is online, apply the mute directly
        self.db.add_mute(target.xuid, int(mute_expiration.timestamp()), reason)
        target.send_message(message)
        sender.send_message(
            f"§6Player §e{player_name} §6was muted for §e\"{reason}\" §6which expires §e{formatted_expiration}")
    else:
        # If the player is offline, use xuid to mute them
        self.db.add_mute(self.db.get_xuid_by_name(player_name), int(mute_expiration.timestamp()), reason)
        sender.send_message(
            f"§6Player §e{player_name} §6was muted for §e\"{reason}\" §6which expires §e{formatted_expiration} §7§o(Offline)")

    log(self, f"§6Player §e{player_name} §6was muted by §e{sender.name} §6for §e\"{reason}\" §6for §e{formatted_expiration}", "mod")

    return True
