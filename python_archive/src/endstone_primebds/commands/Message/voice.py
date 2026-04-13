from endstone.command import CommandSender
from endstone_primebds.utils.command_util import create_command
import endstone_primebds.utils.internal_permissions_util as perms_util

from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from endstone_primebds.primebds import PrimeBDS

# Register command
command, permission = create_command(
    "voice",
    "Grant or revoke exemption from the globalmute command!",
    ["/voice <player: player> [enabled: bool]"],
    ["primebds.command.voice"]
)

def handler(self: "PrimeBDS", sender: CommandSender, args: list[str]) -> bool:
    target_name = args[0]
    player = self.server.get_player(target_name)

    if not player:
        sender.send_message(f"§cPlayer {target_name} not found")
        return False

    enabled = not player.has_permission("primebds.globalmute.exempt")
    if len(args) > 1:
        arg = args[1].lower()
        if arg in ("false", "0", "off", "no"):
            enabled = False
        elif arg in ("true", "1", "on", "yes"):
            enabled = True

    player.add_attachment(self, "primebds.globalmute.exempt", enabled)
    perms_util.invalidate_perm_cache(player.xuid)

    if enabled:
        sender.send_message(f"§e{player.name} §6is now exempt from global mutes")
        player.send_message("§6You are now exempt from the global mute")
    else:
        sender.send_message(f"§e{player.name} §6is no longer exempt from global mutes")
        player.send_message("§6You are no longer exempt from global mutes")

    return True
