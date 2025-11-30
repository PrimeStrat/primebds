import math
from endstone import Player
from endstone.command import CommandSender
from endstone_primebds.utils.command_util import create_command
from endstone_primebds.utils.config_util import load_config
from endstone_primebds.utils.internal_permissions_util import get_permission_header
from endstone_primebds.utils.economy_utils import get_eco_link
from endstone._internal.endstone_python import Location

from typing import TYPE_CHECKING
if TYPE_CHECKING:
    from endstone_primebds.primebds import PrimeBDS

import random
from time import time

command, permission = create_command(
    "rtp",
    "Randomly teleport to a new location!",
    ["/rtp"],
    ["primebds.command.rtp"],
    "op",
    ["randomtp", "wild", "rt"]
)

# delay & cooldown tracking
rtp_cooldowns: dict[str, float] = {}
rtp_delays: dict[str, bool] = {}
rtp_tasks: dict[str, int] = {}

def handler(self: "PrimeBDS", sender: CommandSender, args: list[str]) -> bool:
    if not isinstance(sender, Player):
        sender.send_message("§cThis command can only be executed by a player")
        return False

    config = load_config()
    mod = config["modules"]["rtp"]

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

    if eco and cost > 0:
        bal = eco.api_get_player_money(sender.name)
        if bal < cost:
            sender.send_message(f"§cYou need §e{cost} coins§c to use /rtp (you have {bal}).")
            return False

    now = time()
    last_used = rtp_cooldowns.get(sender.id, 0)

    if rtp_delays.get(sender.id, False):
        sender.send_message("§cYou are already in an RTP warmup!")
        return False

    if now - last_used < cooldown:
        remaining = cooldown - (now - last_used)
        sender.send_message(f"§cYou must wait {remaining:.1f}s before using /rtp again")
        return False

    rx = None
    rz = None
    ry = None

    MAX_ATTEMPTS = 40
    ATTEMPT_SEPARATION = 12 

    attempted_points = [] 
    rx = rz = ry = None

    for _ in range(MAX_ATTEMPTS):
        angle = random.random() * math.tau
        dist = random.uniform(min_dist, radius)
        x = cx + math.cos(angle) * dist
        z = cz + math.sin(angle) * dist

        too_close_to_attempt = False
        for ax, az in attempted_points:
            if ((x - ax) ** 2 + (z - az) ** 2) ** 0.5 < ATTEMPT_SEPARATION:
                too_close_to_attempt = True
                break

        if too_close_to_attempt:
            continue

        attempted_points.append((x, z))

        try:
            y = dim.get_highest_block_y_at(x, z)
            rx, rz, ry = x, z, y
            break
        except:
            continue

    if rx is None:
        sender.send_message("§cFailed to find a valid teleport location. Try again.")
        return False

    if delay <= 0:
        try:
            if eco and cost > 0:
                if eco_name == "umoney":
                    eco.api_change_player_money(sender.name, -cost)

            sender.teleport(Location(dim, rx, ry, rz, sender.location.pitch, sender.location.yaw))
            sender.send_message(f"§aRandomly teleported to §e{rx:.1f}, {ry:.1f}, {rz:.1f}")
            rtp_cooldowns[sender.id] = time()
        except Exception:
            sender.send_message("§cFailed to perform random teleport")
        return True

    rtp_delays[sender.id] = True
    start_pos = sender.location
    start_time = time()

    def repeated_check():
        if sender.location.distance(start_pos) > 0.25:
            sender.send_message("§cTeleport cancelled because you moved!")
            rtp_delays[sender.id] = False
            task_id = rtp_tasks.pop(sender.id, None)
            if task_id:
                self.server.scheduler.cancel_task(task_id)
            return True

        remaining = max(0, delay - (time() - start_time))
        sender.send_popup(f"§aRTP in §e{remaining:.1f}s")

        if remaining <= 0:
            try:
                if eco and cost > 0:
                    if eco_name == "umoney":
                        eco.api_change_player_money(sender.name, -cost)

                sender.teleport(Location(dim, rx, ry, rz, sender.location.pitch, sender.location.yaw))
                sender.send_message(f"§aRandomly teleported to §e{rx:.1f}, {ry:.1f}, {rz:.1f}")
                rtp_cooldowns[sender.id] = time()
            except Exception:
                sender.send_message("§cFailed to perform random teleport")

            rtp_delays[sender.id] = False
            task_id = rtp_tasks.pop(sender.id, None)
            if task_id:
                self.server.scheduler.cancel_task(task_id)
            return True

        return False

    task = self.server.scheduler.run_task(self, repeated_check, delay=0, period=20)
    rtp_tasks[sender.id] = task.task_id
    return True
