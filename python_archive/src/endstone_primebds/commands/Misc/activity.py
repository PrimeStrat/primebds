import time
from endstone import Player
from endstone.command import CommandSender
from endstone_primebds.utils.command_util import create_command

from endstone_primebds.utils.time_util import TimezoneUtils
from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from endstone_primebds.primebds import PrimeBDS

# Register command
command, permission = create_command(
    "activity",
    "Lists out session information!",
    ["/activity <player: player> [page: int]"],
    ["primebds.command.activity"]
)

SESSIONS_PER_PAGE = 5

def handler(self: "PrimeBDS", sender: CommandSender, args: list[str]) -> bool:
    if not isinstance(sender, Player):
        sender.send_message(f"This command can only be executed by a player")
        return False
        
    if len(args) < 1:
        sender.sendMessage(f" Usage: /activity <player> [page: int]")
        return True
    
    if any("@" in arg for arg in args):
        sender.send_message(f"§cTarget selectors are invalid for this command")
        return False

    player_name = args[0]
    target = self.server.get_player(player_name)
    page = int(args[1]) if len(args) > 1 and args[1].isdigit() else 1
    if page < 1:
        page = 1

    xuid = self.db.get_xuid_by_name(player_name)

    if not xuid:
        sender.send_message(f"No session history found for {player_name}")
        return True

    # Fetch all user sessions
    sessions = self.sldb.get_user_sessions(xuid)
    if not sessions:
        sender.send_message(f"No session history found for {player_name}")
        return True

    total_playtime_seconds = self.sldb.get_total_playtime(xuid)
    playtime_str = format_time(total_playtime_seconds)

    sessions.sort(key=lambda s: s['start_time'], reverse=True)
    total_pages = (len(sessions) + SESSIONS_PER_PAGE - 1) // SESSIONS_PER_PAGE
    start_idx = (page - 1) * SESSIONS_PER_PAGE
    end_idx = start_idx + SESSIONS_PER_PAGE
    paginated_sessions = sessions[start_idx:end_idx]

    sender.send_message(f"§bSession History for §e{player_name} §7(Page {page}/{total_pages}):")
    sender.send_message(f"§eTotal Playtime: §f{playtime_str}")

    active_session = next((s for s in sessions if s['end_time'] is None), None)

    if active_session:
        if target:
            start_time_str = TimezoneUtils.convert_to_timezone(active_session['start_time'], 'EST')
            if not start_time_str.startswith("CORRUPTED"):
                active_seconds = int(time.time() - active_session['start_time'])
                sender.send_message(
                    f"§7- §a{start_time_str}§7 - §aActive Now §f(+{format_time(active_seconds)})"
                )
            sessions.remove(active_session)
        else:
            now = int(time.time())
            MAX_SESSION_LENGTH = 3600 

            if self.last_shutdown_time and self.last_shutdown_time > active_session['start_time']:
                candidate_end = self.last_shutdown_time
            else:
                candidate_end = now

            max_allowed_end = active_session['start_time'] + MAX_SESSION_LENGTH
            if candidate_end > max_allowed_end:
                candidate_end = max_allowed_end

            self.sldb.end_session(xuid, candidate_end)
            active_session['end_time'] = candidate_end
            active_session['duration'] = candidate_end - active_session['start_time']

    # Paginate session history
    for session in paginated_sessions:
        start_str = TimezoneUtils.convert_to_timezone(session['start_time'], 'EST')
        end_str = TimezoneUtils.convert_to_timezone(session['end_time'], 'EST')
        duration_text = f"§f({format_time(session['duration'])})"
        if start_str is not None and end_str is not None:
            sender.send_message(f"§7- §a{start_str}§7 - §c{end_str} {duration_text}")
    
    return True

def format_time(seconds: int) -> str:
    if seconds < 60:
        return f"{seconds}s"
    minutes = seconds // 60
    if minutes < 60:
        return f"{minutes}m"
    hours = minutes // 60
    if hours < 24:
        return f"{hours}h"
    days = hours // 24
    return f"{days}d"