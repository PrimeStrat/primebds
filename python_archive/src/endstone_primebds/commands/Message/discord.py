from endstone.command import CommandSender
from endstone_primebds.utils.command_util import create_command
from endstone_primebds.utils.config_util import load_config

from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from endstone_primebds.primebds import PrimeBDS

# Register command
command, permission = create_command(
    "discord",
    "View this server's discord link!",
    ["/discord"],
    ["primebds.command.discord"]
)

config = load_config()
def handler(self: "PrimeBDS", sender: CommandSender, args: list[str]) -> bool:
    sender.send_message(f"{config['modules']['discord']['command']}")
    return True
