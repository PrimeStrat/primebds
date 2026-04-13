from endstone.command import CommandSender
try:
    from endstone.command import BlockCommandSender
except ImportError:
    BlockCommandSender = None 
from endstone_primebds.utils.command_util import create_command

from endstone_primebds.utils.logging_util import log
from endstone_primebds.utils.mod_util import format_time_remaining, ban_message, safe_duration
from datetime import timedelta, datetime
from endstone_primebds.utils.address_util import is_valid_ip

from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from endstone_primebds.primebds import PrimeBDS

# Register command
command, permission = create_command(
    "ipban",
    "Bans a player's IP from the server, either temporarily or permanently!",
    [
        "/ipban <player: player> <duration_number: int> (second|minute|hour|day|week|month|year)<duration_length: ip_ban_length> [reason: message]",
        "/ipban <player: player> (forever)<perm_ban: perm_ban> [reason: message]",
        "/ipban <player: player> (ip)<perm_ipban: perm_ipban> [reason: message]"
    ],
    ["primebds.command.ipban"]
)

# IPBAN COMMAND FUNCTIONALITY
def handler(self: "PrimeBDS", sender: CommandSender, args: list[str]) -> bool:
    if BlockCommandSender is not None and isinstance(sender, BlockCommandSender):
        sender.send_message("§cThis command cannot be automated")
        return False

    if any("@" in arg for arg in args):
        sender.send_message(f"§cTarget selectors are invalid for this command")
        return False

    if len(args) > 1 and args[1].lower() == "ip":
        ip = args[0]

        if not is_valid_ip(ip):
            sender.send_message(f"§6Not a valid IP address")
            return False

        reason = "Negative Behavior"
        if len(args) > 2:
            reason = " ".join(args[2:])

        ban_expiration = int((datetime.now() + timedelta(days=365*200)).timestamp())
        formatted_expiration = "Never - Permanent Ban"
        message = ban_message(self.server.level.name, formatted_expiration, "IP Ban - " + reason)

        full_ip = ip
        for player in self.server.online_players:
            if player.address.hostname == ip:
                full_ip = str(player.address)
                player.kick(message)

        self.db.add_ip_ban(full_ip, ban_expiration, reason)

        sender.send_message(f"§6IP §e{ip} §6was permanently banned for §e'{reason}' §7§o(IP Banned)")
        log(self, f"§6IP §e{ip} §6was permanently IP banned by §e{sender.name} for §e\"{reason}\"", "mod")
        return True

    player_name = args[0].strip('"')
    target = self.server.get_player(player_name)

    # Check if the player is already IP-banned (online or offline)
    if target:
        if self.db.get_mod_log(target.xuid).is_ip_banned:
            sender.send_message(f"§6Player §e{player_name} §6is already IP-banned")
            return False
    else:
        mod_log = self.db.get_offline_mod_log(player_name)
        if mod_log and mod_log.is_ip_banned:
            sender.send_message(f"§6Player §e{player_name} §6is already IP-banned")
            return False
        if not mod_log:
            sender.send_message(f"§6Player '{player_name}' not found")
            return False

    reason = "Negative Behavior"
    permanent = args[1].lower() == "forever"

    if permanent:
        ban_expiration = datetime.now() + timedelta(days=365 * 200)
        reason = " ".join(args[2:]) if len(args) > 2 else reason
    else:
        if len(args) < 3:
            sender.send_message(f"Invalid duration format. Use an integer followed by a time unit")
            return False

        try:
            duration_number = int(args[1])
            if duration_number < 0:
                sender.send_message(f"§6Duration must be a positive number")
                return False
            duration_unit = args[2].lower()
        except ValueError:
            sender.send_message(f"Invalid duration format. Use an integer followed by a time unit")
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
            sender.send_message(f"Invalid time unit. Use: second, minute, hour, day, week, month, year")
            return False

        ban_duration = safe_duration(time_units[duration_unit].total_seconds())
        ban_expiration = datetime.now() + ban_duration
        reason = " ".join(args[3:]) if len(args) > 3 else reason

    formatted_expiration = "Never - Permanent Ban" if permanent else format_time_remaining(int(ban_expiration.timestamp()))
    message = ban_message(self.server.level.name, formatted_expiration, "IP Ban - " + reason)

    target_user = target if target else self.db.get_offline_mod_log(player_name)
    if not target_user:
        sender.send_message(f"Could not retrieve IP for '{player_name}'")
        return False

    self.db.add_ban(target_user.xuid, int(ban_expiration.timestamp()), reason, True)

    if target:
        target.kick(message)
        sender.send_message(
            f"§6Player §e{player_name} §6was {'permanently ' if permanent else ''}IP banned for §e'{reason}' §6{'(Permanent IP Banned)' if permanent else f'for {formatted_expiration}'}"
        )
    else:
        sender.send_message(
            f"§6Player §e{player_name} §6was {'permanently ' if permanent else ''}IP banned for §e'{reason}' §6{'(Offline, Permanent IP Banned)' if permanent else f'for {formatted_expiration}'}"
        )

    # Kick alts
    alts = self.db.get_alts(str(target_user.ip_address), target_user.device_id, target_user.xuid)
    for alt in alts:
        name = alt["name"]
        if name.lower() == target_user.name.lower():
            continue
        user = self.server.get_player(name)
        if user:
            user.kick(message)

    log(self,
        f"§6Player §e{player_name} §6was IP banned by §e{sender.name} for §e\"{reason}\" until §e{formatted_expiration}",
        "mod"
    )
    return True
