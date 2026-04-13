import os
import json
from math import ceil

from endstone.command import CommandSender
from endstone_primebds.utils.command_util import create_command

from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from endstone_primebds.primebds import PrimeBDS

command, permission = create_command(
    "filterlist",
    "Lists all players with a filter!",
    ["/filterlist (ops|default|online|offline|muted|banned|ipbanned)<plist_filter: plist_filter> [page: int]"],
    ["primebds.command.filterlist"],
    "op",
    ["flist"]
)

MAX_PER_PAGE = 10

def handler(self: "PrimeBDS", sender: CommandSender, args: list[str]) -> bool:
    filter_type = args[0].lower()
    page = int(args[1]) if len(args) > 1 and args[1].isdigit() else 1

    def get_ops():
        permissions_path = get_permissions_path()
        if not os.path.exists(permissions_path):
            return ["§cpermissions.json not found"]

        try:
            with open(permissions_path, 'r') as f:
                data = json.load(f)

            ops = [entry["xuid"] for entry in data if entry.get("permission") == "operator"]
            names = []
            for xuid in ops:
                name = self.db.get_name_by_xuid(xuid) or "§8Unknown"
                names.append(f"§e{name} §8({xuid})")
            return sorted(names, key=lambda n: (n.startswith("§8Unknown"), n.lower()))
        except Exception as e:
            return [f"§cFailed to read permissions.json: {e}"]

    def get_default():
        return [
            p['name']
            for p in self.db.get_all_users()
            if p.get('internal_rank', '').lower() == "default"
        ]

    def get_online():
        return [pl.name for pl in self.server.online_players]

    def get_offline():
        online = {pl.name for pl in self.server.online_players}
        return [p['name'] for p in self.db.get_all_users() if p['name'] not in online]

    def get_muted():
        """Return a list of currently muted player names (expired mutes auto-removed)."""
        muted_players = []

        for user in self.db.get_all_users():
            name = user['name']
            xuid = user['xuid']
            mod_log = self.db.get_offline_mod_log(name)

            if mod_log and mod_log.is_muted:
                is_still_muted = self.db.check_and_update_mute(xuid, name)
                if is_still_muted:
                    muted_players.append(name)

        return muted_players

    def get_banned():
        """Return a list of all banned player names with labels for type."""
        banned_players = []

        for user in self.db.get_all_users():
            name = user['name']
            mod_log = self.db.get_offline_mod_log(name)

            if mod_log and getattr(mod_log, "is_banned", False):
                banned_players.append(f"{name} §7(User Banned)")

        try:
            server_bans = self.serverdb.get_all_bans()
            for entry in server_bans:
                banned_name = entry.get("name") if isinstance(entry, dict) else str(entry)
                if banned_name:
                    banned_players.append(f"{banned_name} §7(Name Banned)")
        except Exception as e:
            self.logger.warn(f"Failed to fetch server bans: {e}")

        return banned_players

    def get_ipbanned():
        """Return a list of currently IP-banned player names."""
        ipbanned_players = []

        for user in self.db.get_all_users():
            name = user['name']
            mod_log = self.db.get_offline_mod_log(name)

            if mod_log and getattr(mod_log, "is_ip_banned", False):
                ipbanned_players.append(name)

        return ipbanned_players

    filters = {
        "ops": get_ops,
        "default": get_default,
        "online": get_online,
        "offline": get_offline,
        "muted": get_muted,
        "banned": get_banned,
        "ipbanned": get_ipbanned
    }

    if filter_type not in filters:
        sender.send_message("§cInvalid filter type")
        
        return False

    all_results = filters[filter_type]()
    total = len(all_results)

    if total == 0:
        sender.send_message(f"§7No {filter_type} players found")
        
        return True

    total_pages = ceil(total / MAX_PER_PAGE)
    if page < 1 or page > total_pages:
        sender.send_message(f"§cInvalid page number. Available pages: 1-{total_pages}")
        
        return False

    start_idx = (page - 1) * MAX_PER_PAGE
    end_idx = start_idx + MAX_PER_PAGE
    results = all_results[start_idx:end_idx]

    header = f"§r{filter_type.capitalize()} Players §7(Page {page}/{total_pages}):"
    body = "\n".join(f"§7- §e{name}" for name in results)
    sender.send_message(header + "\n" + body)

    return True

def get_permissions_path():
    current_dir = os.path.dirname(os.path.abspath(__file__))
    while not (
        os.path.exists(os.path.join(current_dir, 'plugins')) and
        os.path.exists(os.path.join(current_dir, 'worlds'))
    ):
        current_dir = os.path.dirname(current_dir)
    return os.path.join(current_dir, 'permissions.json')
