from endstone.command import CommandSender
try:
    from endstone.command import BlockCommandSender
except ImportError:
    BlockCommandSender = None 
from endstone_primebds.utils.command_util import create_command

from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from endstone_primebds.primebds import PrimeBDS

# Register command
command, permission = create_command(
    "clearchat",
    "Adds 100 empty lines to chat!",
    ["/clearchat"],
    ["primebds.command.clearchat"]
)

def handler(self: "PrimeBDS", sender: CommandSender, args: list[str]) -> bool:
    if BlockCommandSender is not None and isinstance(sender, BlockCommandSender):
       sender.send_message("§cThis command cannot be automated")
       return False



    empty_lines = 100
    for player in self.server.online_players:
        player.send_message("\n" * empty_lines)
        player.send_message("§cGlobal chat was cleared")
