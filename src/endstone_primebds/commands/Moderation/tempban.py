from endstone import ColorFormat
from endstone.command import CommandSender
from endstone_primebds.utils.commandUtil import create_command
from endstone_primebds.utils.configUtil import load_config
from endstone_primebds.utils.dbUtil import UserDB
from endstone_primebds.utils.loggingUtil import log
from endstone_primebds.utils.modUtil import format_time_remaining, ban_message
from datetime import timedelta, datetime

from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from endstone_primebds.primebds import PrimeBDS

# Register command
command, permission = create_command(
    "tempban",
    "Temporarily bans a player from the server!",
    ["/tempban <player: player> <duration_number: int> (second|minute|hour|day|week|month|year)<duration_length: ban_length> [reason: message]"],
    ["primebds.command.tempban"]
)

# TEMPBAN COMMAND FUNCTIONALITY
def handler(self: "PrimeBDS", sender: CommandSender, args: list[str]) -> bool:
    if len(args) < 3:
        sender.send_message(f"Usage: /tempban <player> <duration_number> (second|minute|hour|day|week|month|year) [reason]")
        return False
    
    if any("@" in arg for arg in args):
        sender.send_message(f"§c@ selectors are invalid for this command")
        return False

    player_name = args[0].strip('"')
    target = self.server.get_player(player_name)

    db = UserDB("users.db")
    if not target:
        # If the player is offline, look them up by name in the database
        mod_log = db.get_offline_mod_log(player_name)
        if not mod_log:
            sender.send_message(f"Player {ColorFormat.YELLOW}{player_name} not found.")
            db.close_connection()
            return False
        # Check if the player is already banned
        if mod_log.is_banned:
            sender.send_message(f"Player {ColorFormat.YELLOW}{player_name} is already banned.")
            db.close_connection()
            return False

    try:
        duration_number = int(args[1])
        duration_unit = args[2].lower()
    except ValueError:
        sender.send_message(f"Invalid duration format. Use an integer followed by a time unit.")
        return False

    # Supported time units
    time_units = {
        "second": timedelta(seconds=duration_number),
        "minute": timedelta(minutes=duration_number),
        "hour": timedelta(hours=duration_number),
        "day": timedelta(days=duration_number),
        "week": timedelta(weeks=duration_number),
        "month": timedelta(days=30 * duration_number),  # Approximation
        "year": timedelta(days=361 * duration_number)   # Approximation
    }

    if duration_unit not in time_units:
        sender.send_message(f"Invalid time unit. Use: second, minute, hour, day, week, month, year.")
        return False

    ban_duration = time_units[duration_unit]
    ban_expiration = datetime.now() + ban_duration
    reason = " ".join(args[3:]) if len(args) > 3 else "Negative Behavior"

    # Convert datetime to timestamp for format_time_remaining
    formatted_expiration = format_time_remaining(int(ban_expiration.timestamp()))
    message = ban_message(self.server.level.name, formatted_expiration, reason)

    if target:
        # If the player is online, add ban directly
        if db.get_mod_log(target.xuid).is_banned:  # Check if the player is already banned
            sender.send_message(f"Player {ColorFormat.YELLOW}{player_name} is already banned.")
            db.close_connection()
            return False
        db.add_ban(target.xuid, int(ban_expiration.timestamp()), reason)
        target.kick(message)
        sender.send_message(f"Player {ColorFormat.YELLOW}{player_name} {ColorFormat.GOLD}was banned for {ColorFormat.YELLOW}\"{reason}\" {ColorFormat.GOLD}for {ColorFormat.YELLOW}{formatted_expiration}")
    else:
        # If the player is offline, use xuid to ban them
        xuid = db.get_xuid_by_name(player_name)
        if db.get_mod_log(xuid).is_banned:  # Check if the player is already banned
            sender.send_message(f"Player {ColorFormat.YELLOW}{player_name} is already banned.")
            db.close_connection()
            return False
        db.add_ban(xuid, int(ban_expiration.timestamp()), reason)
        sender.send_message(f"Player {ColorFormat.YELLOW}{player_name} {ColorFormat.GOLD}was banned for {ColorFormat.YELLOW}\"{reason}\" {ColorFormat.GOLD}for {ColorFormat.YELLOW}{formatted_expiration} {ColorFormat.GRAY}{ColorFormat.ITALIC}(Offline)")

    config = load_config()
    mod_log_enabled = config["modules"]["game_logging"]["moderation"]["enabled"]
    if mod_log_enabled:
        log(self, f"Player {ColorFormat.YELLOW}{player_name} {ColorFormat.GOLD}was banned by {ColorFormat.YELLOW}{sender.name} {ColorFormat.GOLD}for {ColorFormat.YELLOW}\"{reason}\" {ColorFormat.GOLD}until {ColorFormat.YELLOW}{formatted_expiration}", "mod")

    db.close_connection()
    return True
