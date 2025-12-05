import math
import random
from time import time
from typing import TYPE_CHECKING

from endstone import Player
from endstone.command import CommandSender
from endstone_primebds.utils.command_util import create_command
from endstone_primebds.utils.config_util import load_config
from endstone_primebds.utils.internal_permissions_util import get_permission_header
from endstone_primebds.utils.economy_utils import get_eco_link
from endstone._internal.endstone_python import Location

if TYPE_CHECKING:
    from endstone_primebds.primebds import PrimeBDS

command, permission = create_command(
    "rtp",
    "Randomly teleport to a new location!",
    ["/rtp"],
    ["primebds.command.rtp"],
    "op",
    ["randomtp", "wild", "rt"]
)

# Cooldowns & warmup state
rtp_cooldowns: dict[str, float] = {}
rtp_delays: dict[str, bool] = {}
rtp_tasks: dict[str, int] = {}

def handler(self: "PrimeBDS", sender: CommandSender, args: list[str]) -> bool:
    if not isinstance(sender, Player):
        sender.send_message("§cThis command can only be executed by a player")
        return False

    config = load_config()
    mod = config["modules"]["rtp"]

    # Config values
    cx = mod["x"]
    cz = mod["z"]
    min_dist = mod["min_distance"]
    radius = mod["radius"]
    delay = mod["delay"]
    cooldown = mod["cooldown"]
    cost = mod["cost"]

    dim = sender.dimension
    if dim is None:
        sender.send_message("§cFailed to determine your dimension")
        return False

    eco = get_eco_link(self)
    eco_name = get_permission_header(eco) if eco else None

    # ───── Economy Check ─────
    if eco and cost > 0:
        bal = eco.api_get_player_money(sender.name)
        if bal < cost:
            sender.send_message(f"§cYou need §e{cost} coins§c to use /rtp (you have {bal}).")
            return False

    # ───── Cooldown Check ─────
    now = time()
    last_used = rtp_cooldowns.get(sender.id, 0)

    if rtp_delays.get(sender.id, False):
        sender.send_message("§cYou are already in an RTP warmup!")
        return False

    if now - last_used < cooldown:
        remaining = cooldown - (now - last_used)
        sender.send_message(f"§cYou must wait {remaining:.1f}s before using /rtp again")
        return False
    
    MAX_ATTEMPTS = 40
    ATTEMPT_SEPARATION = max(6, radius * 0.03)

    attempted_points = []
    rx = rz = ry = None

    for _ in range(MAX_ATTEMPTS):
        angle = random.random() * math.tau
        dist = random.uniform(min_dist, radius)
        x = cx + math.cos(angle) * dist
        z = cz + math.sin(angle) * dist

        # Avoid sampling too close to previous points
        if any(
            ((x - ax) ** 2 + (z - az) ** 2) ** 0.5 < ATTEMPT_SEPARATION
            for ax, az in attempted_points
        ):
            continue

        attempted_points.append((x, z))

        try:
            y = dim.get_highest_block_y_at(math.floor(x), math.floor(z))
            rx, rz, ry = x, z, y
            break
        except Exception:
            continue

    if rx is None:
        sender.send_message("§cFailed to find a valid teleport location. Try again.")
        return False

    def perform_teleport():
        """Deduct cost, perform teleport, trigger post-corrections."""
        try:
            if eco and cost > 0 and eco_name == "umoney":
                eco.api_change_player_money(sender.name, -cost)

            sender.teleport(Location(dim, rx, ry, rz, sender.location.pitch, sender.location.yaw))
            sender.send_message(f"§aRandomly teleported to §e{rx:.1f}, {ry:.1f}, {rz:.1f}")
            rtp_cooldowns[sender.id] = time()

            # Run post correction cycles
            run_post_corrections(self, sender, dim)

        except Exception:
            sender.send_message("§cFailed to perform random teleport")

    if delay <= 0:
        perform_teleport()
        return True

    rtp_delays[sender.id] = True
    start_pos = sender.location
    start_time = time()

    def warmup_tick():
        # Cancel if player moved
        if sender.location.distance(start_pos) > 0.25:
            sender.send_message("§cTeleport cancelled because you moved!")
            rtp_delays[sender.id] = False
            task_id = rtp_tasks.pop(sender.id, None)
            if task_id:
                self.server.scheduler.cancel_task(task_id)
            return True

        remaining = max(0, delay - (time() - start_time))
        sender.send_popup(f"§aRTP in §e{remaining:.1f}s")

        # End warmup
        if remaining <= 0:
            perform_teleport()
            rtp_delays[sender.id] = False

            task_id = rtp_tasks.pop(sender.id, None)
            if task_id:
                self.server.scheduler.cancel_task(task_id)

            return True

        return False

    task = self.server.scheduler.run_task(self, warmup_tick, delay=0, period=20)
    rtp_tasks[sender.id] = task.task_id

    return True

def run_post_corrections(self: "PrimeBDS", sender: Player, dim, attempts=0):
    """Re-run highest-block checks after the initial teleport.
    Up to 3 attempts spaced by ~2 ticks.
    """

    if attempts >= 3:
        return True  # done

    try:
        lx = math.floor(sender.location.x)
        lz = math.floor(sender.location.z)

        new_y = dim.get_highest_block_y_at(lx, lz)
        current_y = sender.location.y

        if abs(new_y - current_y) > 0.5:
            sender.teleport(
                Location(
                    dim,
                    sender.location.x,
                    new_y,
                    sender.location.z,
                    sender.location.pitch,
                    sender.location.yaw
                )
            )

        # Schedule next correction
        self.server.scheduler.run_task(
            self,
            lambda: run_post_corrections(self, sender, dim, attempts + 1),
            delay=2
        )

    except Exception:
        pass

    return True
