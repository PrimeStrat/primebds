from endstone.command import CommandSender
try:
    from endstone.command import BlockCommandSender
except ImportError:
    BlockCommandSender = None 
from endstone_primebds.utils.command_util import create_command

from endstone_primebds.utils.logging_util import log
from endstone_primebds.utils.mod_util import format_time_remaining, ban_message, safe_duration
from datetime import timedelta, datetime

from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from endstone_primebds.primebds import PrimeBDS

# Register command
command, permission = create_command(
    "nameban",
    "Bans a player's name from the server, either temporarily or permanently!",
    [
        "/nameban <player: player> <duration_number: int> (second|minute|hour|day|week|month|year)<duration_length: name_ban_length> [reason: message]",
        "/nameban <player: player> (forever)<name_ban: name_ban> [reason: message]"
    ],
    ["primebds.command.nameban"]
)

# PERMBAN COMMAND FUNCTIONALITY
def handler(self: "PrimeBDS", sender: CommandSender, args: list[str]) -> bool:
    if BlockCommandSender is not None and isinstance(sender, BlockCommandSender):
       sender.send_message("§cThis command cannot be automated")
       return False

    if any("@" in arg for arg in args):
        sender.send_message(f"§cTarget selectors are invalid for this command")
        return False

    player_name = args[0].strip('"')

    if self.serverdb.check_nameban(player_name):
        sender.send_message(
            f"§6Player §e{player_name} §cis already name banned")
        return False

    reason = "Negative Behavior"
    permanent = args[1].lower() == "forever"

    if permanent:
        ban_expiration = datetime.now() + timedelta(days=365 * 200)
        reason = " ".join(args[2:]) if len(args) > 2 else reason
    else:
        if len(args) < 3:
            sender.send_message(f"Invalid duration format. Use an integer followed by a time unit")
            return False

        try:
            duration_number = int(args[1])

            if duration_number < 0:
                sender.send_message(f"§6Duration must be a positive number")
                return False

            duration_unit = args[2].lower()
        except ValueError:
            sender.send_message(f"Invalid duration format. Use an integer followed by a time unit")
            return False

        # Supported time units
        time_units = {
            "second": timedelta(seconds=duration_number),
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

        ban_duration = safe_duration(time_units[duration_unit].total_seconds())
        ban_expiration = datetime.now() + ban_duration
        reason = " ".join(args[3:]) if len(args) > 3 else reason

    formatted_expiration = "Never - Permanent Ban" if permanent else format_time_remaining(int(ban_expiration.timestamp()))
    message = ban_message(self.server.level.name, formatted_expiration, "Ban - " + reason)

    self.serverdb.add_name(player_name, reason, int(ban_expiration.timestamp()))

    target = self.server.get_player(player_name)
    if target:
        target.kick(message)
        if permanent:
            sender.send_message(
                f"§6Player §e{player_name} §6was permanently name banned for §e'{reason}' §7§o(Permanent Name Ban)"
            )
        else:
            sender.send_message(
                f"§6Player §e{player_name} §6was name banned for §e'{reason}' §6for {formatted_expiration} §7§o(Name Banned)"
            )
    else:
        if permanent:
            sender.send_message(
                f"§6Player §e{player_name} §6was permanently name banned for §e'{reason}' §7§o(Offline, Permanent Name Ban)"
            )
        else:
            sender.send_message(
                f"§6Player §e{player_name} §6was name banned for §e'{reason}' §6for {formatted_expiration} §7§o(Offline, Name Banned)"
            )

    log(self,
            f"§6Player §e{player_name} §6was name banned by §e{sender.name} for §e\"{reason}\" until §e{formatted_expiration}",
            "mod")

    return True