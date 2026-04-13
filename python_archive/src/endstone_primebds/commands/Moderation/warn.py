import time
from endstone.command import CommandSender
try:
    from endstone.command import BlockCommandSender
except ImportError:
    BlockCommandSender = None 
from endstone_primebds.utils.command_util import create_command
from endstone_primebds.utils.mod_util import format_time_remaining
from endstone_primebds.utils.logging_util import log

from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from endstone_primebds.primebds import PrimeBDS

# Register command
command, permission = create_command(
    "warn",
    "Warn a player that they are breaking a rule!",
    ["/warn <player: player> <reason: string> [duration_number: int] (second|minute|hour|day|week|month|year)[duration_length: warn_length]"],
    ["primebds.command.warn"]
)

def handler(self: "PrimeBDS", sender: CommandSender, args: list[str]) -> bool:
    if BlockCommandSender is not None and isinstance(sender, BlockCommandSender):
       sender.send_message("§cThis command cannot be automated")
       return False



    if len(args) < 2:
        sender.send_message("§cUsage: /warn <player> <reason> [duration_number] [duration_length]")
        return False

    target_name = args[0]
    target_xuid = self.db.get_xuid_by_name(target_name)

    if not target_xuid:
        sender.send_message(f"§cPlayer '{target_name}' not found")
        return False

    duration_seconds = 200 * 31536000
    reason_parts = args[1:]

    if len(args) >= 4 and args[-2].isdigit():
        number = int(args[-2])
        unit = args[-1].lower()

        unit_seconds = {
            "second": 1,
            "minute": 60,
            "hour": 3600,
            "day": 86400,
            "week": 604800,
            "month": 2592000,
            "year": 31536000
        }

        if unit in unit_seconds:
            duration_seconds = number * unit_seconds[unit]
            reason_parts = args[1:-2]

    reason = " ".join(reason_parts).strip()

    if not reason:
        sender.send_message("§cYou must provide a reason for the warning")
        return False

    self.db.add_warning(
        reason,
        sender.name,
        duration_seconds,
        target_xuid,
        target_name
    )

    if duration_seconds > 0:
        expiration_str = format_time_remaining(int(time.time()) + duration_seconds, True)
        sender.send_message(f"§6Player §e{target_name} §6was warned for §e\"{reason}\" §6which expires §e{expiration_str}")
    else:
        sender.send_message(f"§6Player §e{target_name} §6was warned for §e\"{reason}\" §6which is §e{expiration_str}")

    log(self, f"§6Player §e{target_name} §6was warned by §e{sender.name} §6for §e\"{reason}\" §6which expires §e{expiration_str}", "mod")

    return True

