from collections import Counter
import math
from endstone import Player
from endstone.command import CommandSender
try:
    from endstone.command import BlockCommandSender
except ImportError:
    BlockCommandSender = None 
from endstone_primebds.utils.command_util import create_command
from endstone_primebds.utils.target_selector_util import get_target_entity

from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from endstone_primebds.primebds import PrimeBDS

# Register command
command, permission = create_command(
    "entityinfo",
    "Check entity information!",
    [
        "/entityinfo (list)[entity_action: entity_action] [page: int]"
    ],
    ["primebds.command.entityinfo"]
)

def handler(self: "PrimeBDS", sender: CommandSender, args: list[str]) -> bool:
    if isinstance(sender, BlockCommandSender):
        sender.send_message("§cThis command cannot be automated")
        return False

    if any("@" in arg for arg in args):
        sender.send_message("§cTarget selectors are invalid for this command")
        return False

    entities = self.server.level.actors

    if len(args) == 0:
        if not isinstance(sender, Player):
            sender.send_message("This command can only be executed by a player")
            return False
        actor = get_target_entity(sender)
        if actor != None:
            sender.send_message(f"""§bEntity Information:
§7- §eName: §f"{getattr(actor, "name_tag", None) or "Unset"}"
§7- §eType: §f{actor.type}
§7- §eUnique ID: §f{actor.id}
§7- §eRuntime ID: §f{actor.runtime_id}
§7- §eLocation: §fx: {actor.location.block_x} §7/ §fy: {actor.location.block_y} §7/ §fz: {actor.location.block_z}
§7- §eRotation: §fyaw: {round(actor.location.yaw, 2)} §7/ §fpitch: {round(actor.location.pitch, 2)}
§7- §eDimension: §f{actor.dimension.name}
§7- §eHealth: §f{actor.health}/{actor.max_health}
§7- §eGrounded: §f{actor.is_on_ground}
§7- §eIn Lava: §f{actor.is_in_lava}
§7- §eIn Water: §f{actor.is_in_water}
§7- §eTags: §f{actor.scoreboard_tags}
""")
        else:
            sender.send_message(f"No entities found 10 blocks in front of you")

    page = 1

    # LIST ALL TYPES mode
    if args and args[0].lower() == "list":
        if len(args) > 1 and args[1].isdigit():
            page = max(1, int(args[1]))

        type_counts = Counter(e.type for e in entities)
        sorted_types = sorted(type_counts.items(), key=lambda x: x[1], reverse=True)

        total_pages = math.ceil(len(sorted_types) / 10)
        start_idx = (page - 1) * 10
        end_idx = start_idx + 10
        page_items = sorted_types[start_idx:end_idx]

        sender.send_message(f"§eEntity Summary §7(Page {page}/{total_pages}):")
        sender.send_message(f"§eTotal: §f{len(entities)}")
        for entity_type, count in page_items:
            sender.send_message(f"  §7- §e{entity_type}: §f{count}")

        return True

