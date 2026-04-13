from math import sin, cos, radians
from endstone import Player
from endstone.command import CommandSender
from endstone_primebds.utils.command_util import create_command

from typing import TYPE_CHECKING
if TYPE_CHECKING:
    from endstone_primebds.primebds import PrimeBDS

command, permission = create_command(
    "blockinfo",
    "Prints info of the facing block!",
    ["/blockinfo [location: pos]"],
    ["primebds.command.blockinfo"]
)

def handler(self: "PrimeBDS", sender: CommandSender, args: list[str]) -> bool:
    if not isinstance(sender, Player):
        sender.send_message("§cOnly players can use this command.")
        return True

    player = sender
    dim = player.dimension

    if len(args) >= 1:
        pos_str = args[0] 
        x_arg, y_arg, z_arg = pos_str.split()
        player = sender 
        px, py, pz = player.location.x, player.location.y, player.location.z

        def resolve_coord(arg: str, base: float) -> float:
            arg = arg.strip()
            if arg.startswith("~"):
                if arg == "~":
                    return base
                else:
                    return base + float(arg[1:])
            return float(arg)

        x = resolve_coord(x_arg, px)
        y = resolve_coord(y_arg, py)
        z = resolve_coord(z_arg, pz)

        block = dim.get_block_at(int(x), int(y), int(z))
        b = block.location

        sender.send_message(f"""§bBlock Information:
§7- §eX: §f{b.block_x}
§7- §eY: §f{b.block_y}
§7- §eZ: §f{b.block_z}
§7- §eType: §f{block.type}
§7- §eStates: §f{block.data.block_states}
§7- §eRuntime ID: §f{block.data.runtime_id}
""")
        return True

    base = player.location
    eye_x = base.x
    eye_y = base.y
    eye_z = base.z

    yaw = radians(base.yaw)
    pitch = radians(base.pitch)

    dir_x = -sin(yaw) * cos(pitch)
    dir_y = -sin(pitch)
    dir_z =  cos(yaw) * cos(pitch)

    max_distance = 6
    step = 0.2
    distance = 0.0
    hit_block = None

    while distance <= max_distance:
        cx = eye_x + dir_x * distance
        cy = eye_y + dir_y * distance
        cz = eye_z + dir_z * distance
        block = dim.get_block_at(int(cx), int(cy), int(cz))

        if not block.type == "minecraft:air":
            hit_block = block
            break

        distance += step

    if hit_block:
        b = hit_block.location

        sender.send_message(f"""§bBlock Information:
§7- §eX: §f{b.block_x}
§7- §eY: §f{b.block_y}
§7- §eZ: §f{b.block_z}
§7- §eType: §f{hit_block.type}
§7- §eStates: §f{hit_block.data.block_states}
§7- §eRuntime ID: §f{hit_block.data.runtime_id}
""")
    else:
        sender.send_message("§cNo block in sight")

    return True
