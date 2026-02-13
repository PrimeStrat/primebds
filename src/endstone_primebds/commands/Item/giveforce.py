from endstone.command import CommandSender
from endstone.inventory import ItemStack
from endstone_primebds.utils.command_util import create_command
from endstone_primebds.utils.target_selector_util import get_matching_actors

try:
    from endstone._internal.endstone_python import NamespacedKey
    HAS_NAMESPACEDKEY = True
except ImportError:
    HAS_NAMESPACEDKEY = False

from typing import TYPE_CHECKING
if TYPE_CHECKING:
    from endstone_primebds.primebds import PrimeBDS

# Register command
command, permission = create_command(
    "giveforce",
    "Forces any item registered to be given, hidden or not!",
    [
        "/giveforce <player: player> <blockItem: block> [amount: int] [data: int]",
        "/giveforce <player: player> <item: string> [amount: int] [data: int]",
        "/giveforce <player: player> (glowingobsidian|netherreactor|portal|end_portal|end_gateway|frosted_ice|enchanted_book|spawn_egg|info_update|info_update2|reserved6|unknown|client_request_placeholder_block|moving_block|stonecutter|camera|glow_stick)<hiddenItem: hiddenItem> [amount: int] [data: int]"
    ],
    ["primebds.command.giveforce"],
    "op",
    ["givef", "forcegive"]
)

def handler(self: "PrimeBDS", sender: CommandSender, args: list[str]) -> bool:
    if len(args) < 2:
        sender.send_message("§cUsage: /giveforce <player> <block> [amount]")
        return False

    target_selector = args[0]
    block_id = args[1].lower()
    amount = 1
    data = 0
    
    if len(args) >= 3:
        try:
            amount = int(args[2])
        except ValueError:
            sender.send_message("§cInvalid amount")
            return False

        if amount < 1:
            sender.send_message("§cAmount must be a positive integer")
            return False
        if amount > 255:
            sender.send_message("§cAmount must be 255 or lower")
            return False
    
    if len(args) >= 4:
        data = int(args[3])

    targets = get_matching_actors(self, target_selector, sender)
    if not targets:
        sender.send_message("§cNo matching players found")
        return False
    
    if "invisiblebedrock" in block_id:
        block_id = "invisible_bedrock"

    try:
        block = ItemStack(block_id, amount, data)
    except Exception:
        sender.send_message("§cInvalid block ID")
        return False
    
    for target in targets:
        target.inventory.add_item(block)

    if len(targets) == 1:
        sender.send_message(f"§e{targets[0].name} §rwas given §7x{amount} §e{block.type.key}")
    else:
        sender.send_message(f"§e{len(targets)} §rplayers were given §7x{amount} §e{block.type.key}")

    return True
