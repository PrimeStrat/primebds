from endstone import Player
from endstone.command import CommandSender
from endstone_primebds.utils.command_util import create_command
from typing import TYPE_CHECKING
import math

if TYPE_CHECKING:
    from endstone_primebds.primebds import PrimeBDS

# Command registration
command, permission = create_command(
    "blockscan",
    "Continuously show information about the block you're looking at.",
    ["/blockscan (disable)[blockscan: blockscan]"],
    ["primebds.command.blockscan"]
)

def handler(self: "PrimeBDS", sender: CommandSender, args: list[str]) -> bool:
    if not isinstance(sender, Player):
        sender.send_message("§cOnly players can use this command")
        return True

    player_name = sender.name

    if not hasattr(self, "blockscan_intervals"):
        self.blockscan_intervals = {}

    if player_name in self.blockscan_intervals:
        self.server.scheduler.cancel_task(self.blockscan_intervals[player_name])
        del self.blockscan_intervals[player_name]
        sender.send_message("§cBlock scanning disabled")
        return True

    interval_seconds = 0.5

    def get_direction(player: Player):
        pitch = math.radians(player.location.pitch)
        yaw = math.radians(player.location.yaw)

        dir_x = -math.sin(yaw) * math.cos(pitch)
        dir_y = -math.sin(pitch)
        dir_z = math.cos(yaw) * math.cos(pitch)

        return (dir_x, dir_y, dir_z)

    def scan_interval(player_name: str):
        player = self.server.get_player(player_name)
        if not player:
            if player_name in self.blockscan_intervals:
                self.server.scheduler.cancel_task(self.blockscan_intervals[player_name])
                del self.blockscan_intervals[player_name]
            return

        dim = player.dimension
        loc = player.location
        dir_x, dir_y, dir_z = get_direction(player)

        eye_x = loc.x
        eye_y = loc.y + 1
        eye_z = loc.z

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
            block_type = hit_block.type
            block_states = hit_block.data.block_states
            runtime_id = hit_block.data.runtime_id

            if isinstance(block_states, dict):
                states_list = [f"{k} = {v}" for k, v in block_states.items()]
            else:
                states_list = []
                for entry in block_states:
                    for k, v in entry.items():
                        states_list.append(f"{k} = {v}")

            states_text = "\n".join(f"  §7- §f{s}" for s in states_list) if states_list else "  §7- (none)"

            player.send_tip(
                f"§bBlock Information§r\n"
                f"§7- §eX: §f{b.block_x}\n"
                f"§7- §eY: §f{b.block_y}\n"
                f"§7- §eZ: §f{b.block_z}\n"
                f"§7- §eType: §f{block_type}\n"
                f"§7- §eStates:\n{states_text}\n"
                f"§7- §eRuntime ID: §f{runtime_id}"
            )
        else:
            player.send_tip("§cNo block in sight")

    task = self.server.scheduler.run_task(
        self,
        lambda: scan_interval(player_name),
        delay=0,
        period=4
    )

    if task:
        self.blockscan_intervals[player_name] = task.task_id
        sender.send_message("§aBlock scanning enabled")
    else:
        sender.send_message("§cFailed to start block scanning task")

    return True

def clear_all_blockscan_intervals(self: "PrimeBDS"):
    """Clear all active intervals."""
    for player_name, task_id in self.blockscan_intervals.items():
        self.server.scheduler.cancel_task(task_id) 
    self.blockscan_intervals.clear()