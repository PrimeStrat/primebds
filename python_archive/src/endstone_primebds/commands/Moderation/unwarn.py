import time
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
    "unwarn",
    "Remove a warning or clear all warnings from a player!",
    ["/unwarn <player: player> (clear)<warn_action: warn_action>",
     "/unwarn <player: player> [id: int]"],
    ["primebds.command.unwarn"]
)

def handler(self: "PrimeBDS", sender: CommandSender, args: list[str]) -> bool:
    if BlockCommandSender is not None and isinstance(sender, BlockCommandSender):
       sender.send_message("§cThis command cannot be automated")
       return False



    if len(args) < 1:
        sender.send_message("§cUsage: /unwarn <player> (clear) | /unwarn <player> <id>")
        return False

    target_name = args[0]
    target_xuid = self.db.get_xuid_by_name(target_name)

    if not target_xuid:
        sender.send_message(f"§cPlayer '{target_name}' not found.")
        return False

    if len(args) == 1:
        now = int(time.time())
        permanent_threshold = 31536000 * 100  # 100 years in seconds

        # Find latest active warning
        last_warning = self.db.execute(
            """
            SELECT id, warn_time
            FROM warn_logs
            WHERE xuid = ?
            ORDER BY warn_time DESC
            LIMIT 1
            """,
            (target_xuid,),
            readonly=True
        ).fetchone()

        if not last_warning:
            sender.send_message(f"§cNo active warnings found for player §e{target_name}")
            return False

        warn_id, warn_time = last_warning
        is_permanent = warn_time >= now + permanent_threshold
        is_active = is_permanent or warn_time > now

        if not is_active:
            sender.send_message(f"§6No active warnings found for player §e{target_name}")
            return False

        success = self.db.execute(
            "UPDATE warn_logs SET warn_time = ? WHERE id = ?",
            (now, warn_id)
        )

        if success.rowcount > 0:
            sender.send_message(f"§6Warning §eID {warn_id} §6was pardoned for player §e{target_name}")
            log(self, f"§e{sender.name} §6pardoned warning §eID {warn_id} §6for §e{target_name}", "mod")
            return True
        else:
            sender.send_message(f"§6Failed to expire warning ID §e{warn_id} §cfor player §e{target_name}")
            return False

    action = args[1].lower()

    if action == "clear":
        active_warnings = self.db.get_warnings(target_xuid)

        if not active_warnings:
            sender.send_message(f"§e{target_name} §6has no active warnings to clear")
            return True

        self.db.expire_warnings(target_xuid)
        sender.send_message(f"§6All active warnings for §e{target_name} §6have been cleared")
        log(self, f"§e{sender.name} §6cleared all active warnings for §e{target_name}", "mod")
        return True

    try:
        warn_id = int(action)
    except ValueError:
        sender.send_message("§cInvalid warning ID. Please specify a valid number or 'clear'")
        return False

    success = self.db.expire_warning_by_id(warn_id)
    if success:
        sender.send_message(f"§6Warning §eID {warn_id} §6pardoned for player §e{target_name}")
        log(self, f"§e{sender.name} §6pardoned warning §eID {warn_id} 6for §e{target_name}", "mod")
    else:
        sender.send_message(f"§cWarning ID §e{warn_id} §cnot found for player §e{target_name}")

    return True
