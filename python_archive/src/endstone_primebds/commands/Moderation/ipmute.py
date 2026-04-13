from endstone.command import CommandSender
try:
    from endstone.command import BlockCommandSender
except ImportError:
    BlockCommandSender = None 
from endstone_primebds.utils.command_util import create_command

from endstone_primebds.utils.logging_util import log
from endstone_primebds.utils.mod_util import format_time_remaining
from datetime import timedelta, datetime
from endstone_primebds.utils.address_util import is_valid_ip

from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from endstone_primebds.primebds import PrimeBDS

# Register command
command, permission = create_command(
    "ipmute",
    "Permanently or temporarily mutes a player based on their IP!",
    [
        "/ipmute <player: player> <duration_number: int> (second|minute|hour|day|week|month|year)<duration_length: ip_mute_length> [reason: message]",
        "/ipmute <player: player> (forever)<perm_mute: perm_mute> [reason: message]"
    ],
    ["primebds.command.mute"],
    "op",
    ["mute-ip"]
)

def handler(self: "PrimeBDS", sender: CommandSender, args: list[str]) -> bool:
    if BlockCommandSender is not None and isinstance(sender, BlockCommandSender):
       sender.send_message("§cThis command cannot be automated")
       return False

    if any("@" in arg for arg in args):
        sender.send_message("§cTarget selectors are invalid for this command")
        return False

    if len(args) < 2:
        sender.send_message("§cUsage: /ipmute <player> <duration_number> <unit> [reason] or /ipmute <player> forever [reason]")
        return False
    
    if is_valid_ip(args[0].strip('"')):
        sender.send_message(f"§cThis override only supports known player targets")
        return False

    player_name = args[0].strip('"')
    target = self.server.get_player(player_name)

    if target:
        target_user = self.db.get_mod_log(target.xuid)
    else:
        target_user = self.db.get_offline_mod_log(player_name)

    if not target_user:
        sender.send_message(f"§cPlayer '{player_name}' not found")
        return False

    if target_user.is_muted:
        formatted_expiration = format_time_remaining(target_user.mute_time, True)
        sender.send_message(
            f"§6Player §e{player_name} §6is already muted for §e{target_user.mute_reason}§6, expires §e{formatted_expiration}"
        )
        return False

    reason = "Negative Behavior"
    permanent = args[1].lower() == "forever"

    if permanent:
        mute_expiration = datetime.now() + timedelta(days=365 * 200)
        reason = " ".join(args[2:]) if len(args) > 2 else reason
    else:
        if len(args) < 3:
            sender.send_message("§cInvalid duration. Use an integer followed by a time unit")
            return False

        try:
            duration_number = int(args[1])
            if duration_number < 0:
                sender.send_message("§cDuration must be positive")
                return False
            duration_unit = args[2].lower()
        except ValueError:
            sender.send_message("§cInvalid duration format")
            return False

        time_units = {
            "second": timedelta(seconds=duration_number),
            "minute": timedelta(minutes=duration_number),
            "hour": timedelta(hours=duration_number),
            "day": timedelta(days=duration_number),
            "week": timedelta(weeks=duration_number),
            "month": timedelta(days=30 * duration_number),  # Approx.
            "year": timedelta(days=365 * duration_number),  # Approx.
        }

        if duration_unit not in time_units:
            sender.send_message("§cInvalid time unit. Use: second, minute, hour, day, week, month, year")
            return False

        mute_duration = time_units[duration_unit]
        mute_expiration = datetime.now() + mute_duration
        reason = " ".join(args[3:]) if len(args) > 3 else reason

    formatted_expiration = "Never - Permanent Mute" if permanent else format_time_remaining(int(mute_expiration.timestamp()), True)
    message = f"§6You are muted for §e{reason} §6which expires §e{formatted_expiration}"

    self.db.add_mute(target_user.xuid, int(mute_expiration.timestamp()), reason, True)
    if target:
        target.send_message(message)

    sender.send_message(
        f"§6Player §e{player_name} §6was IP-muted for §e\"{reason}\" §6which expires §e{formatted_expiration}"
    )

    log(
        self,
        f"§6Player §e{player_name} §6was IP-muted by §e{sender.name} §6for §e\"{reason}\" §6until §e{formatted_expiration}",
        "mod"
    )

    return True
