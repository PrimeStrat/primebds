from endstone import Player
from endstone.command import CommandSender
from endstone_primebds.utils.config_util import load_config
from endstone_primebds.utils.command_util import create_command

from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from endstone_primebds.primebds import PrimeBDS

# Register command
command, permission = create_command(
    "staffchat",
    "Allows you to chat in a staff-only chat channel!",
    ["/staffchat [toggle: bool]",
     "/staffchat <message: message>"],
    ["primebds.command.staffchat"],
    "op",
    ["sc"]
)

# STAFFCHAT COMMAND FUNCTIONALITY
def handler(self: "PrimeBDS", sender: CommandSender, args: list[str]) -> bool:
    if not isinstance(sender, Player):
        sender.send_error_message("This command can only be executed by a player")
        return False

    user = self.db.get_online_user(sender.xuid)

    if not args or args[0] == "":
        current_status = int(user.enabled_sc)
        new_status = 0 if current_status == 1 else 1
        self.db.update_user_data(sender.name, "enabled_sc", new_status)
        sender.send_message(f"§6Staff Chat has been {f'§aEnabled' if new_status == 1 else f'§cDisabled'}")
    else:
        arg = args[0].lower()
        # Check if first arg is a toggle command
        if arg in ["true", "1", "yes", "enable", "false", "0", "no", "disable"]:
            new_status = 1 if arg in ["true", "1", "yes", "enable"] else 0
            self.db.update_user_data(sender.name, "enabled_sc", new_status)
            sender.send_message(f"§6Staff Chat has been {f'§aEnabled' if new_status == 1 else f'§cDisabled'}")
        else:
            config = load_config()
            message = f"{config['modules']['server_messages']['staff_chat_prefix']}{sender.name}: {args[0]}"
            self.server.broadcast(message, "primebds.command.staffchat")

    return True
