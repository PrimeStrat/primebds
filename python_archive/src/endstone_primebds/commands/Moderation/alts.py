from endstone import ColorFormat
from endstone.command import CommandSender
try:
    from endstone.command import BlockCommandSender
except ImportError:
    BlockCommandSender = None 
from endstone_primebds.utils.command_util import create_command
from endstone_primebds.utils.address_util import same_subnet

from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from endstone_primebds.primebds import PrimeBDS

# Register command
command, permission = create_command(
    "alts",
    "Checks known alternate accounts for a player.",
    ["/alts <player: player>"],
    ["primebds.command.alts"]
)

def handler(self: "PrimeBDS", sender: CommandSender, args: list[str]) -> bool:
    if BlockCommandSender is not None and isinstance(sender, BlockCommandSender):
       sender.send_message("§cThis command cannot be automated")
       return False

    if any("@" in arg for arg in args):
        sender.send_message("§cTarget selectors are not allowed for this command")
        return False

    player_name = args[0].strip('"')
    target = sender.server.get_player(player_name)

    if target is None:
        mod_log = self.db.get_offline_mod_log(player_name)
        user = self.db.get_offline_user(player_name)
        if user is None or mod_log is None:
            sender.send_message(f"§cPlayer §e{player_name}§c not found in database")
            return False

        ip = mod_log.ip_address
        device_id = user.device_id
        target_xuid = user.xuid
        target_status = f"{ColorFormat.RED}Offline"
    else:
        mod_log = self.db.get_mod_log(target.xuid)
        ip = str(target.address)
        device_id = target.device_id
        target_xuid = target.xuid
        target_status = f"{ColorFormat.GREEN}Online"

    alts = self.db.get_alts(ip, device_id, target_xuid)

    if not alts:
        sender.send_message(
            f"§6No alternate accounts found for §e{player_name}"
        )
        return True

    alt_lines = []
    for alt in alts:
        if not alt: 
            continue

        name = alt.get("name")
        last_ip = alt.get("ip_address")
        dev_id = alt.get("device_id")

        if not name and not last_ip and not dev_id:
            continue

        tags = []
        if ip and last_ip and same_subnet(ip, last_ip):
            tags.append("IP")
        if device_id and dev_id and dev_id == device_id:
            tags.append("Device")
        
        if tags: 
            alt_lines.append(f"{name or 'Unknown'} §7[{' & '.join(tags)} match§7]")

    if not alt_lines:
        sender.send_message(
            f"§6No alternate accounts found for §e{player_name}"
        )
        return True

    alt_list = "\n§7  - §f".join(alt_lines)
    sender.send_message(f"""§6Alternate Accounts:
§7- §ePlayer: §f{player_name} §8[{target_status}§8]
§7- §eAlts: §8[§e{len(alt_lines)}§8]
§7  - §f{alt_list}
""")
    return True
