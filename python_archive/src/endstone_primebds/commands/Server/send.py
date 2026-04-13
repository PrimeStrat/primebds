from endstone import Player
from endstone.command import CommandSender
from endstone_primebds.utils.command_util import create_command
from endstone_primebds.utils.target_selector_util import get_matching_actors
from endstone_primebds.utils.address_util import is_valid_ip, is_valid_port
from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from endstone_primebds.primebds import PrimeBDS

# Register command
command, permission = create_command(
    "send",
    "Sends players to a specified server address!",
    ["/send <player: player> <ip: string> [port: int]"],
    ["primebds.command.send"]
)

# CONNECT COMMAND FUNCTIONALITY
def handler(self: "PrimeBDS", sender: CommandSender, args: list[str]) -> bool:
    target = args[0]
    ip = args[1]
    port_str = args[2] if len(args) >= 3 else "19132"

    if not is_valid_ip(ip):
        sender.send_message(f"Invalid IP address: {ip}")
        return False

    if not is_valid_port(port_str):
        sender.send_message(f"Invalid port number: {port_str}")
        return False

    port = int(port_str)

    targets = get_matching_actors(self, target, sender)
    for target in targets:
        if target.is_valid and isinstance(target, Player):
            target.transfer(ip, port)
        else:
            sender.send_message(f"Target player is invalid")

    return True