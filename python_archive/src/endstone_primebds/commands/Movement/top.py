from endstone import Player
from endstone.command import CommandSender
from endstone_primebds.utils.command_util import create_command


from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from endstone_primebds.primebds import PrimeBDS

# Register command
command, permission = create_command(
    "top",
    "Warps you to the topmost block with air!",
    ["/top"],
    ["primebds.command.top"]
)

# TOP COMMAND FUNCTIONALITY
def handler(self: "PrimeBDS", sender: CommandSender, args: list[str]) -> bool:
    if not isinstance(sender, Player):
        sender.send_error_message(f"This command can only be executed by a player.")
        return False

    player = self.server.get_player(sender.name)
    pos = player.location
    dimension = player.dimension

    # Set world height limits based on dimension
    world_height = 120 if dimension.name == "Nether" else 255 if dimension.name == "TheEnd" else 320  # Overworld
    min_y = -63  # Default min_y to -63 if not provided
    x, z = int(pos.x), int(pos.z)

    # Get the highest block Y level at the given X, Z position
    highest_y = min(dimension.get_highest_block_y_at(x, z), world_height)

    if highest_y < min_y:
        sender.send_message(f"No valid open-air block found at this X, Z position.")
        return False

    # Check for valid teleport spot by ensuring at least 2 air blocks above
    for y in range(highest_y, max(min_y, highest_y - 5), -1):  # Small downward search if needed
        if (
            dimension.get_block_at(x, y + 1, z).type == "minecraft:air" and
            dimension.get_block_at(x, y + 2, z).type == "minecraft:air"
        ):
            player.perform_command(f"tp {x} {y + 1} {z}")
            return True

    sender.send_message(f"No valid open-air block found at this X, Z position.")
    return False
