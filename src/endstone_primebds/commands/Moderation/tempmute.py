from endstone import ColorFormat
from endstone.command import CommandSender
from endstone_primebds.utils.commandUtil import create_command
from endstone_primebds.utils.configUtil import load_config
from endstone_primebds.utils.dbUtil import UserDB
from endstone_primebds.utils.loggingUtil import log
from endstone_primebds.utils.modUtil import format_time_remaining
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
    if len(args) < 3:
        sender.send_message(f"Usage: /tempmute <player> <duration_number> (second|minute|hour|day|week|month|year) [reason]")
        return False
    
    if any("@" in arg for arg in args):
        sender.send_message(f"§c@ selectors are invalid for this command")
        return False

    player_name = args[0].strip('"')
    target = self.server.get_player(player_name)

    db = UserDB("users.db")
    if not target:
        mod_log = db.get_offline_mod_log(player_name)
        if not mod_log:
            sender.send_message(f"Player {ColorFormat.YELLOW}{player_name} not found.")
            db.close_connection()
            return False
        if mod_log.is_muted:
            sender.send_message(f"Player {ColorFormat.YELLOW}{player_name} is already muted.")
            db.close_connection()
            return False

    try:
        duration_number = int(args[1])
        duration_unit = args[2].lower()
    except ValueError:
        sender.send_message(f"Invalid duration format. Use an integer followed by a time unit.")
        return False

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
        sender.send_message(f"Invalid time unit. Use: second, minute, hour, day, week, month, year.")
        return False

    mute_duration = time_units[duration_unit]
    mute_expiration = datetime.now() + mute_duration
    reason = " ".join(args[3:]) if len(args) > 3 else "Disruptive Behavior"

    formatted_expiration = format_time_remaining(int(mute_expiration.timestamp()))
    message = f"You are muted for {ColorFormat.YELLOW}\"{reason}\" {ColorFormat.GOLD}which expires in {ColorFormat.YELLOW}{formatted_expiration}"

    if target:
        if db.get_mod_log(target.xuid).is_muted:
            sender.send_message(f"Player {ColorFormat.YELLOW}{player_name} is already muted.")
            db.close_connection()
            return False
        target.send_message(message)
        db.add_mute(target.xuid, int(mute_expiration.timestamp()), reason)
        sender.send_message(f"Player {ColorFormat.YELLOW}{player_name} {ColorFormat.GOLD}was muted for {ColorFormat.YELLOW}\"{reason}\" {ColorFormat.GOLD}for {ColorFormat.YELLOW}{formatted_expiration}")
    else:
        xuid = db.get_xuid_by_name(player_name)
        if db.get_mod_log(xuid).is_muted:
            sender.send_message(f"Player {ColorFormat.YELLOW}{player_name} is already muted.")
            db.close_connection()
            return False
        db.add_mute(xuid, int(mute_expiration.timestamp()), reason)
        sender.send_message(f"Player {ColorFormat.YELLOW}{player_name} {ColorFormat.GOLD}was muted for {ColorFormat.YELLOW}\"{reason}\" {ColorFormat.GOLD}for {ColorFormat.YELLOW}{formatted_expiration} {ColorFormat.GRAY}{ColorFormat.ITALIC}(Offline)")

    config = load_config()
    mod_log_enabled = config["modules"]["game_logging"]["moderation"]["enabled"]
    if mod_log_enabled:
        log(self, f"Player {ColorFormat.YELLOW}{player_name} {ColorFormat.GOLD}was muted by {ColorFormat.YELLOW}{sender.name} {ColorFormat.GOLD}for {ColorFormat.YELLOW}\"{reason}\" {ColorFormat.GOLD}until {ColorFormat.YELLOW}{formatted_expiration}", "mod")

    db.close_connection()
    return True
