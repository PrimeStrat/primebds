from endstone.command import CommandSender
try:
    from endstone.command import BlockCommandSender
except ImportError:
    BlockCommandSender = None 
from endstone_primebds.utils.command_util import create_command

from endstone_primebds.utils.logging_util import log
from endstone_primebds.utils.mod_util import format_time_remaining, safe_duration
from datetime import timedelta, datetime

from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from endstone_primebds.primebds import PrimeBDS

# Register command
command, permission = create_command(
    "tempmute",
    "Temporarily mutes a player on the server!",
    ["/tempmute <player: player> <duration_number: int> (second|minute|hour|day|week|month|year)<duration_length: mute_length> [reason: message]"],
    ["primebds.command.tempmute"]
)

# TEMPMUTE COMMAND FUNCTIONALITY
def handler(self: "PrimeBDS", sender: CommandSender, args: list[str]) -> bool:
    if BlockCommandSender is not None and isinstance(sender, BlockCommandSender):
       sender.send_message("§cThis command cannot be automated")
       return False



    if len(args) < 3:
        sender.send_message(f"Usage: /tempmute <player> <duration_number> (second|minute|hour|day|week|month|year) [reason]")
        return False
    
    if any("@" in arg for arg in args):
        sender.send_message(f"§cTarget selectors are invalid for this command")
        return False

    player_name = args[0].strip('"')
    target = self.server.get_player(player_name)
    
    if not target:
        mod_log = self.db.get_offline_mod_log(player_name)
        if not mod_log:
            sender.send_message(f"§6Player §e{player_name} not found")
            
            return False
        if mod_log.is_muted:
            sender.send_message(f"§6Player §e{player_name} is already muted")
            
            return False
        
    self.db.check_and_update_mute(target.xuid, target.name)

    try:
        duration_number = int(args[1])

        if duration_number < 0:
            sender.send_message(f"§6Duration must be a positive number")
            return False

        duration_unit = args[2].lower()
    except ValueError:
        sender.send_message(f"Invalid duration format. Use an integer followed by a time unit")
        return False

    time_units = {
        "second": timedelta(seconds=duration_number+1),
        "minute": timedelta(minutes=duration_number),
        "hour": timedelta(hours=duration_number),
        "day": timedelta(days=duration_number),
        "week": timedelta(weeks=duration_number),
        "month": timedelta(days=30 * duration_number),
        "year": timedelta(days=361 * duration_number)
    }

    if duration_unit not in time_units:
        sender.send_message(f"Invalid time unit. Use: second, minute, hour, day, week, month, year")
        return False

    mute_duration = safe_duration(time_units[duration_unit].total_seconds())
    mute_expiration = datetime.now() + mute_duration
    reason = " ".join(args[3:]) if len(args) > 3 else "Disruptive Behavior"

    formatted_expiration = format_time_remaining(int(mute_expiration.timestamp()))
    message = f"§6You are muted for §e\"{reason}\" §6which expires in §e{formatted_expiration}"

    if target:
        if self.db.get_mod_log(target.xuid).is_muted:
            sender.send_message(f"§6Player §e{player_name} is already muted")
            
            return False
        target.send_message(message)
        self.db.add_mute(target.xuid, int(mute_expiration.timestamp()), reason)
        sender.send_message(f"§6Player §e{player_name} §6was muted for §e\"{reason}\" §6for §e{formatted_expiration}")
    else:
        xuid = self.db.get_xuid_by_name(player_name)
        if self.db.get_mod_log(xuid).is_muted:
            sender.send_message(f"§6Player §e{player_name} is already muted.")
            
            return False
        self.db.add_mute(xuid, int(mute_expiration.timestamp()), reason)
        sender.send_message(f"§6Player §e{player_name} §6was muted for §e\"{reason}\" §6for §e{formatted_expiration} §7§o(Offline)")

    log(self, f"§6Player §e{player_name} §6was muted by §e{sender.name} §6for §e\"{reason}\" §6until §e{formatted_expiration}", "mod")

    
    return True
