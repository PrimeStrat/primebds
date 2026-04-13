from endstone.command import CommandSender
try:
    from endstone.command import BlockCommandSender
except ImportError:
    BlockCommandSender = None 
from endstone_primebds.utils.command_util import create_command
from endstone_primebds.utils.logging_util import log
from endstone_primebds.utils.address_util import is_valid_ip

from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from endstone_primebds.primebds import PrimeBDS

# Register command
command, permission = create_command(
    "removeban",
    "Removes an active ban from a player!",
    [
        "/removeban <player: player>",
        "/removeban <player: player> (ip)<perm_removeban: perm_removeban>"
    ],
    ["primebds.command.removeban", "primebds.command.pardon"]
)

# REMOVEBAN COMMAND FUNCTIONALITY
def handler(self: "PrimeBDS", sender: CommandSender, args: list[str]) -> bool:
    if BlockCommandSender is not None and isinstance(sender, BlockCommandSender):
        sender.send_message("§cThis command cannot be automated")
        return False

    if len(args) < 1:
        sender.send_message(f"Usage: /removeban <player> or /removeban ip <IP>")
        return False

    if any("@" in arg for arg in args):
        sender.send_message(f"§cTarget selectors are invalid for this command")
        return False

    if len(args) > 1 and args[1].lower() == "ip":
        if len(args) < 2:
            sender.send_message(f"Usage: /removeban ip <IP>")
            return False

        ip = args[0]
        if not is_valid_ip(ip):
            sender.send_message(f"§6Not a valid IP address")
            return False

        self.db.remove_ip_ban(ip)
        for player in self.server.online_players:
            if player.address.hostname == ip:
                player.kick("§cYour IP ban has been lifted")

        sender.send_message(f"§6IP §e{ip} §6ban has been removed")
        log(self, f"§6IP §e{ip} §6ban was removed by §e{sender.name}", "mod")
        return True

    player_name = args[0].strip('"')
    mod_log = self.db.get_offline_mod_log(player_name)

    if not mod_log:
        sender.send_message(f"§6Player §e{player_name} §6has no mod log entry")
        return False

    if not (mod_log.is_banned or mod_log.is_ip_banned):
        sender.send_message(f"§6Player §e{player_name} §6is not banned")
        return False

    self.db.remove_ban(player_name)

    sender.send_message(f"§6Player §e{player_name} §6has been unbanned")
    log(self, f"§6Player §e{player_name} §6was unbanned by §e{sender.name}", "mod")
    return True
