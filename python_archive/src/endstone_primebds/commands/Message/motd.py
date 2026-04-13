from endstone.command import CommandSender
from endstone_primebds.utils.command_util import create_command
from endstone_primebds.utils.config_util import load_config

from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from endstone_primebds.primebds import PrimeBDS

# Register command
command, permission = create_command(
    "motd",
    "View this server's message of the day!",
    ["/motd"],
    ["primebds.command.motd"]
)

config = load_config()
def handler(self: "PrimeBDS", sender: CommandSender, args: list[str]) -> bool:
    sender.send_message(f"{config['modules']['message_of_the_day']['message_of_the_day_command']}")
    return True
