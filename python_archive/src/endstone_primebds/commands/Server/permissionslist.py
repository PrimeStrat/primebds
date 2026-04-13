from collections import defaultdict
from endstone import Player
from endstone.command import CommandSender
try:
    from endstone.command import BlockCommandSender
except ImportError:
    BlockCommandSender = None 
from endstone_primebds.utils.command_util import create_command
import endstone_primebds.utils.internal_permissions_util as perms_util

from endstone_primebds.utils.form_wrapper_util import (
    ActionFormData,
    ActionFormResponse,
)

from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from endstone_primebds.primebds import PrimeBDS

# Register command
command, permission = create_command(
    "permissionslist",
    "View the global or player-specific permissions list!",
    [
        f"/permissionslist [player: player]"
    ],
    ["primebds.command.permissionslist"],
    "op",
    ["permslist"]
)

# SETRANK COMMAND FUNCTIONALITY
def handler(self: "PrimeBDS", sender: CommandSender, args: list[str]) -> bool:
    if BlockCommandSender is not None and isinstance(sender, BlockCommandSender):
       sender.send_message("§cThis command cannot be automated")
       return False

    if any("@" in arg for arg in args):
        sender.send_message(f"§cTarget selectors are invalid for this command")
        return False

    def send_permissions_as_text(title: str, body_text: str):
        sender.send_message(f"§6{title}\n{body_text}")

    if len(args) == 0:
        grouped_perms = defaultdict(list)
        for perm in perms_util.MANAGED_PERMISSIONS_LIST:
            prefix = perm.split(".", 1)[0].lower()
            grouped_perms[prefix].append(perm)

        lines = []
        for prefix in sorted(grouped_perms.keys()):
            lines.append(f"§d{prefix}:")
            for perm in sorted(grouped_perms[prefix]):
                lines.append(f"  §7- §f{perm}")
            lines.append("")

        body_text = "\n".join(lines).strip()

        if isinstance(sender, Player):
            form = ActionFormData()
            form.title("Server Permissions List")
            form.body(body_text)
            form.button("Close")
            def submit(player_obj: Player, result: ActionFormResponse):
                pass

            form.show(sender).then(lambda player=sender, result=ActionFormResponse: submit(player, result))
        else:
            send_permissions_as_text("Server Permissions List", body_text)

        return True

    player = self.server.get_player(args[0])
    if not player:
        sender.send_message(f"§cPlayer {args[0]} not found")
        return False

    grouped_perms = defaultdict(list)

    for perm in perms_util.MANAGED_PERMISSIONS_LIST:
        if player.has_permission(perm):
            prefix = perm.split(".", 1)[0].lower()
            grouped_perms[prefix].append(perm)

    lines = []

    for prefix in sorted(grouped_perms.keys()):
        lines.append(f"§d{prefix}:")
        for perm in sorted(grouped_perms[prefix]):
            lines.append(f"  §7- §f{perm}")
        lines.append("")

    body_text = "\n".join(lines).strip()

    if isinstance(sender, Player):
        form = ActionFormData()
        form.title(f"{player.name}'s Permissions")
        form.body(body_text)
        form.button("Close")
        def submit(player_obj: Player, result: ActionFormResponse):
            pass

        form.show(sender).then(lambda player=sender, result=ActionFormResponse: submit(player, result))
    else:
        send_permissions_as_text(f"{player.name}'s Permissions", body_text)

    return True
