from endstone import Player
from endstone.command import CommandSender
from endstone_primebds.utils.command_util import create_command
from endstone_primebds.utils.target_selector_util import get_matching_actors

from typing import TYPE_CHECKING
if TYPE_CHECKING:
    from endstone_primebds.primebds import PrimeBDS

# Register command
command, permission = create_command(
    "feed",
    "Sets player hunger to full!",
    ["/feed [player: player]"],
    ["primebds.command.feed", "primebds.command.feed.other"],
    "op",
    ["eat"]
)

# FEED COMMAND FUNCTIONALITY
def handler(self: "PrimeBDS", sender: CommandSender, args: list[str]) -> bool:
    if len(args) == 0:
        if not isinstance(sender, Player):
            sender.send_message("This command can only be executed by a player")
            return False
        self.server.dispatch_command(self.server.command_sender, f"effect {sender.name} saturation 3 255 true")
        sender.send_message(f'§aYou were fed')
        return True
    
    if not sender.has_permission("primebds.command.feed.other"):
        sender.send_message(f'§cYou do not have permission to feed others')
        return True

    targets = get_matching_actors(self, args[0], sender)
    for target in targets:
        self.server.dispatch_command(self.server.command_sender, f"effect {target.name} saturation 3 255 true")
        target.send_message('§aYou were fed')
        
    if len(targets) == 1:
        sender.send_message(f'§e{targets[0].name} §rwas fed')
    else:
        sender.send_message(f'§e{len(targets)} §rplayers were fed')

    return True
