from endstone import Player
from endstone.command import CommandSender
from endstone_primebds.utils.commandUtil import create_command
from endstone_primebds.utils.dbUtil import UserDB

from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from endstone_primebds.primebds import PrimeBDS

# Register command
command, permission = create_command(
    "socialspy",
    "Allows you to view private messages!",
    ["/socialspy [toggle: bool]"],
    ["primebds.command.socialspy"]
)

# MODSPY COMMAND FUNCTIONALITY
def handler(self: "PrimeBDS", sender: CommandSender, args: list[str]) -> bool:
    if not isinstance(sender, Player):
        sender.send_error_message("This command can only be executed by a player")
        return False

    
    user = self.db.get_online_user(sender.xuid)

    if not args:
        current_status = int(user.enabled_ss)
        new_status = 0 if current_status == 1 else 1
        self.db.update_user_data(sender.name, "enabled_ss", new_status)
        sender.send_message(f"§6Social Spy has been {f'§aEnabled' if new_status == 1 else f'§cDisabled'}")
    else:
        arg = args[0].lower()
        new_status = 1 if arg in ["true", "1", "yes", "enable"] else 0
        self.db.update_user_data(sender.name, "enabled_ss", new_status)
        sender.send_message(f"§6Social Spy has been {f'§aEnabled' if new_status == 1 else f'§cDisabled'}")

    
    return True
