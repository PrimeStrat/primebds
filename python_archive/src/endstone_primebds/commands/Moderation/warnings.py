import time
from endstone.command import CommandSender
try:
    from endstone.command import BlockCommandSender
except ImportError:
    BlockCommandSender = None 
from endstone_primebds.utils.command_util import create_command
from endstone_primebds.utils.mod_util import format_time_remaining

from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from endstone_primebds.primebds import PrimeBDS

# Register command
command, permission = create_command(
    "warnings",
    "List warnings or permanently delete warnings from a player!",
    [
        "/warnings <player: player> [page: int]",
        "/warnings <player: player> (delete|clear)<del_warn: del_warn> [id: int]"
     ],
    ["primebds.command.warnings"]
)

def handler(self: "PrimeBDS", sender: CommandSender, args: list[str]) -> bool:
    if BlockCommandSender is not None and isinstance(sender, BlockCommandSender):
       sender.send_message("§cThis command cannot be automated")
       return False



    if len(args) < 1:
        sender.send_message("§cUsage: /warnings <player> [page:int]")
        return False

    target_name = args[0]
    target_xuid = self.db.get_xuid_by_name(target_name)
    if len(args) >= 2:
        action = args[1].lower()

        if action == "delete":
            if len(args) < 3 or not args[2].isdigit():
                sender.send_message("§cUsage: /warnings <player> delete <warning_id:int>")
                return False
            warn_id = int(args[2])
            success = self.db.delete_warning_by_id(warn_id)
            if success:
                sender.send_message(f"§6Warning §eID {warn_id} §6was erased")
            else:
                sender.send_message(f"§cWarning §eID {warn_id} §cnot found")
            return True

        elif action == "clear":
            self.db.delete_warnings(target_xuid)
            sender.send_message(f"§6All existing warnings for §e{target_name} §6were erased")
            return True

    if not target_xuid:
        sender.send_message(f"§cPlayer '{target_name}' not found")
        return False

    page = 1
    if len(args) >= 2 and args[1].isdigit():
        page = max(1, int(args[1]))

    now = int(time.time())
    permanent_threshold = 31536000 * 100  # 100 years in seconds

    def is_permanent(warn_time):
        return warn_time >= now + permanent_threshold

    all_warnings = self.db.get_warnings(xuid=target_xuid, include_expired=True)

    if not all_warnings:
        sender.send_message(f"§6No warnings for §e{target_name}")
        return True

    all_warnings.sort(key=lambda w: w["warn_time"], reverse=True)

    def format_warning(warn: dict) -> str:
        warn_id = warn.get("id", "Unknown")
        reason = warn.get("warn_reason", "Unknown")
        exp_time = warn.get("warn_time", 0)
        issuer = warn.get("added_by", "Unknown")

        if is_permanent(exp_time):
            expires = "PERMANENT"
        else:
            expires = f"{format_time_remaining(exp_time, True)}"

        return f"§8[§7{warn_id}§8] §f\"{reason}\" §7- §e{issuer} §8[§e{expires}§8]"

    per_page = 5
    total_pages = max(1, (len(all_warnings) + per_page - 1) // per_page)

    if page > total_pages:
        sender.send_message(f"§cPage {page} does not exist. Max pages: {total_pages}")
        return True

    start_index = (page - 1) * per_page
    end_index = start_index + per_page
    page_warnings = all_warnings[start_index:end_index]

    sender.send_message(f"§6Warnings for §e{target_name} §7(Page {page}/{total_pages}):")
    for warn in page_warnings:
        sender.send_message(format_warning(warn))

    return True
