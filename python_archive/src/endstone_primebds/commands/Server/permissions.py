from endstone.command import CommandSender
try:
    from endstone.command import BlockCommandSender
except ImportError:
    BlockCommandSender = None 
from endstone_primebds.utils.command_util import create_command

from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from endstone_primebds.primebds import PrimeBDS

command, permission = create_command(
    "permissions",
    "Sets the internal permissions for a player!",
    [
        "/permissions <player: player> (settrue|setfalse|setneutral)<set_perm: set_perm> <perm: string>"
     ],
    ["primebds.command.permissions"],
    "op",
    ["perms"]
)

# PERMISSIONS COMMAND FUNCTIONALITY
def handler(self: "PrimeBDS", sender: CommandSender, args: list[str]) -> bool:
    if BlockCommandSender is not None and isinstance(sender, BlockCommandSender):
       sender.send_message("§cThis command cannot be automated")
       return False

    if any("@" in arg for arg in args):
        sender.send_message(f"§cTarget selectors are invalid for this command")
        return False

    target = args[0]
    subaction = args[1].lower()

    user = self.db.get_offline_user(target)
    if not user:
        sender.send_message(f"Player \"{target}\" not found")
        return False

    if subaction in "settrue":
        permission = args[2]
        player = self.server.get_player(target)
        self.db.set_permission(user.xuid, permission, True)
        if player:
            self.reload_custom_perms(player)
        sender.send_message(f"§e{permission} §bpermission for §e{target} §bwas set to §atrue")

    elif subaction == "setfalse":
        permission = args[2]
        player = self.server.get_player(target)
        self.db.set_permission(user.xuid, permission, False)
        if player:
            self.reload_custom_perms(player)
        sender.send_message(f"§e{permission} §bpermission for §e{target} §bwas set to §cfalse")

    elif subaction == "setneutral":
        permission = args[2]
        player = self.server.get_player(target)
        self.db.delete_permission(user.xuid, permission)
        if player:
            self.reload_custom_perms(player)

        sender.send_message(f"§e{permission} §bpermission for §e{target} §bwas set to §7neutral")

    return True