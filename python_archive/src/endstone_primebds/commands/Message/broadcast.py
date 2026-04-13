from endstone.command import CommandSender
from endstone_primebds.utils.command_util import create_command
from endstone_primebds.utils.config_util import load_config

from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from endstone_primebds.primebds import PrimeBDS

# Register command
command, permission = create_command(
    "broadcast",
    "Send a server-wide notification!",
    ["/broadcast <message: message>"],
    ["primebds.command.broadcast"]
)

def handler(self: "PrimeBDS", sender: CommandSender, args: list[str]) -> bool:
    config = load_config()
    self.server.broadcast_message(f"{config['modules']['broadcast']['prefix']}{args[0]}")

    if config["modules"]["broadcast"]["playsound"] is not "":
        for player in self.server.online_players:
            player.play_sound(player.location, config["modules"]["broadcast"]["playsound"])
