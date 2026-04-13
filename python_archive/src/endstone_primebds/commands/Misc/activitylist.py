from endstone import Player, ColorFormat
from endstone.command import CommandSender
from endstone_primebds.utils.command_util import create_command
from math import ceil

from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from endstone_primebds.primebds import PrimeBDS

# Register command
command, permission = create_command(
    "activitylist",
    "Lists players by activity filter (highest, lowest, or recent)!",
    ["/activitylist [page: int] (highest|lowest|recent)[filter: filter]"],
    ["primebds.command.activitylist"]
)

# ACTIVITY LIST COMMAND FUNCTIONALITY
def handler(self: "PrimeBDS", sender: CommandSender, args: list[str]) -> bool:
    if not isinstance(sender, Player):
        sender.send_message("This command can only be executed by a player")
        return False

    page = 1
    filter_type = "highest"

    if len(args) >= 1:
        if args[0].isdigit():
            page = int(args[0])
            if len(args) >= 2 and args[1].lower() in ["highest", "lowest", "recent"]:
                filter_type = args[1].lower()
        elif args[0].lower() in ["highest", "lowest", "recent"]:
            filter_type = args[0].lower()
            if len(args) >= 2 and args[1].isdigit():
                page = int(args[1])

    playtimes = self.sldb.get_all_playtimes()
    if not playtimes:
        sender.send_message("No player playtime data found")
        return True

    if filter_type == "highest":
        sorted_playtimes = sorted(playtimes, key=lambda x: x['total_playtime'], reverse=True)
    elif filter_type == "lowest":
        sorted_playtimes = sorted(playtimes, key=lambda x: x['total_playtime'])
    else: 
        sorted_playtimes = []
        for player in playtimes:
            sessions = self.sldb.get_user_sessions(player['xuid'])
            if sessions:
                recent_session = max(sessions, key=lambda s: s['end_time'] or s['start_time'])
                sorted_playtimes.append({
                    'name': player['name'],
                    'xuid': player['xuid'],
                    'recent_session_start': recent_session['start_time'],
                    'total_playtime': player['total_playtime']
                })
        sorted_playtimes = sorted(sorted_playtimes, key=lambda x: x['recent_session_start'], reverse=True)

    per_page = 10
    total_pages = ceil(len(sorted_playtimes) / per_page)
    page = max(1, min(page, total_pages))
    start_idx = (page - 1) * per_page
    end_idx = start_idx + per_page
    page_entries = sorted_playtimes[start_idx:end_idx]

    sender.send_message(f"§bActivity List ({filter_type.capitalize()}) - Page {page}/{total_pages}\n")
    for i, entry in enumerate(page_entries, start=start_idx + 1):
        total_playtime_seconds = entry['total_playtime']
        days = total_playtime_seconds // 86400
        hours = (total_playtime_seconds % 86400) // 3600
        minutes = (total_playtime_seconds % 3600) // 60
        seconds = total_playtime_seconds % 60

        playtime_str = ""
        if days > 0:
            playtime_str += f"{days}d "
        if hours > 0 or days > 0:
            playtime_str += f"{hours}h "
        if minutes > 0 or hours > 0 or days > 0:
            playtime_str += f"{minutes}m "
        playtime_str += f"{seconds}s"

        sender.send_message(f"§7{i}. §e{entry['name']} §7- §c{playtime_str}")

    return True