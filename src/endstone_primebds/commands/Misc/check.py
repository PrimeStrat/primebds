from datetime import datetime
from endstone_primebds.utils.time_util import TimezoneUtils
from endstone_primebds.utils.mod_util import format_time_remaining
from endstone.command import CommandSender
from endstone_primebds.utils.command_util import create_command

from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from endstone_primebds.primebds import PrimeBDS

# Register command
command, permission = create_command(
    "check",
    "Checks a player's client info!",
    ["/check <player: player> (info|mod|network|world)[info: info]"],
    ["primebds.command.check"],
    "op",
    ["seen"]
)

# CHECK COMMAND FUNCTIONALITY
def handler(self: "PrimeBDS", sender: CommandSender, args: list[str]) -> bool:

    if any("@" in arg for arg in args):
        sender.send_message(f"§cTarget selectors are invalid for this command")
        return False

    player_name = args[0].strip('"')
    filter_type = args[1].lower() if len(args) > 1 else "info"  # default to info if not provided
    target = sender.server.get_player(player_name)

    if target is None:
        # Check Offline DB
        user = self.db.get_offline_user(player_name)
        user_mod = self.db.get_offline_mod_log(player_name)
        if user is None:
            sender.send_message(
                f"Player §e{player_name}§c not found in database."
            )
            return False

        xuid = user.xuid
        uuid = user.uuid
        unique_id = user.unique_id
        name = user.name
        ping = f"{user.ping}ms §7[Last Recorded§7]"
        device_os = user.device_os
        device_id = user.device_id
        version = user.client_ver
        rank = user.internal_rank
        last_join = user.last_join
        last_leave = user.last_leave
        status = f"§cOffline"
        is_ip_muted = bool(user_mod.is_ip_muted)
        is_ip_banned = bool(user_mod.is_ip_banned)
        is_banned = bool(user_mod.is_banned)
        ban_reason = user_mod.ban_reason
        ban_time = user_mod.banned_time
        is_muted = bool(user_mod.is_muted)
        mute_reason = user_mod.mute_reason
        mute_time = user_mod.mute_time
        ip = user_mod.ip_address

    else:
        # Fetch Online Data
        user = self.db.get_online_user(target.xuid)
        user_mod = self.db.get_mod_log(target.xuid)
        xuid = target.xuid
        uuid = target.unique_id
        unique_id = user.unique_id
        name = target.name
        ping = f"{target.ping}ms"
        device_os = user.device_os
        device_id = user.device_id
        version = target.game_version
        rank = user.internal_rank
        last_join = user.last_join
        last_leave = user.last_leave
        status = f"§aOnline"
        is_ip_muted = bool(user_mod.is_ip_muted)
        is_ip_banned = bool(user_mod.is_ip_banned)
        is_banned = bool(user_mod.is_banned)
        ban_reason = user_mod.ban_reason
        ban_time = user_mod.banned_time
        is_muted = bool(user_mod.is_muted)
        mute_reason = user_mod.mute_reason
        mute_time = user_mod.mute_time
        ip = str(target.address)

    join_time = TimezoneUtils.convert_to_timezone(last_join, "EST")

    dt = datetime.fromtimestamp(last_leave)
    year = dt.year

    leave_time_str = (
        "N/A"
        if year < 2000
        else TimezoneUtils.convert_to_timezone(last_leave, "EST")
    )

    if filter_type == "info":
        sender.send_message(f"""§bPlayer Database Information:
§7- §eName: §f{name} §8[{status}§8]
§7- §eXUID: §f{xuid}
§7- §eUUID: §f{uuid}
§7- §eUnique ID: §f{unique_id}
§7- §eInternal Rank: §f{rank}
§7- §eDevice OS: §f{device_os}
§7- §eDevice ID: §f{device_id}
§7- §eClient Version: §f{version}
§7- §ePing: §f{ping}
§7- §eLast Join: §f{join_time}
§7- §eLast Leave: §f{leave_time_str}
""")

    elif filter_type == "mod":

        warning = self.db.get_latest_active_warning(None, player_name)

        ban_info = ""
        name_ban_info = ""
        mute_info = ""
        warn_info = ""

        if is_banned:
            ban_info = f"""
§7  - §eBan Reason: §f{ban_reason}
§7  - §eBan Expires: §f{format_time_remaining(ban_time)}
"""
        
        if self.serverdb.check_nameban(player_name):
            name_ban = self.serverdb.get_ban_info(player_name)
            name_ban_info = f"""
§7  - §eBan Reason: §f{name_ban.ban_reason}
§7  - §eBan Expires: §f{format_time_remaining(name_ban.banned_time)}
"""

        if is_muted:
            mute_info = f"""
§7  - §eMute Reason: §f{mute_reason}
§7  - §eMute Expires: §f{format_time_remaining(mute_time, is_mute=True)}
"""
            
        if warning:
            warn_info = f"""
§7  - §eWarn Reason: §f{warning["warn_reason"]}
§7  - §eWarn Expires: §f{format_time_remaining(warning["warn_time"], True)}
"""

        sender.send_message(f"""§6Player Mod Information:
§7- §eName: §f{name} §8[{status}§8]
§7- §eRank: §f{rank}
§7- §eBanned: §f{is_banned} §8[§7IP: {is_ip_banned}§8]{ban_info}
§7- §eName Banned: §f{self.serverdb.check_nameban(player_name)}{name_ban_info}
§7- §eMuted: §f{is_muted} §8[§7IP: {is_ip_muted}§8]{mute_info}
§7- §eWarned: §f{bool(warning)}{warn_info}
""")

    elif filter_type == "network":
        sender.send_message(f"""§dPlayer Network Information:
§7- §eName: §f{name} §8[{status}§8]
§7- §eIP: §f{ip}
""")

    elif filter_type == "world":
        if target:
            sender.send_message(f"""§aPlayer World Information:
§7- §eName: §f{name} §8[{status}§8]
§7- §eUnique ID: §f{target.id}
§7- §eGamemode: §f{target.game_mode.name}
§7- §eLocation: §fx: {target.location.block_x} §7/ §fy: {target.location.block_y} §7/ §fz: {target.location.block_z}
§7- §eRotation: §fyaw: {round(target.location.yaw, 2)} §7/ §fpitch: {round(target.location.pitch, 2)}
§7- §eDimension: §f{target.dimension.name}
§7- §eHealth: §f{target.health}/{target.max_health}
§7- §eTotal XP: §f{target.total_exp}
§7- §eGrounded: §f{target.is_on_ground}
§7- §eIn Lava: §f{target.is_in_lava}
§7- §eIn Water: §f{target.is_in_water}
§7- §eIs OP: §f{target.is_op}
§7- §eCan Fly: §f{target.allow_flight}
§7- §eTags: §f{target.scoreboard_tags}
""")
        else:
            sender.send_message(f"§cPlayer must be online to check world data")

    else:
        sender.send_message(f"§cInvalid filter type '{filter_type}'")
        return False

    return True
