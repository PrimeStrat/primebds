from endstone import Player
from endstone.command import CommandSender
from endstone_primebds.utils.command_util import create_command
from time import time
from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from endstone_primebds.primebds import PrimeBDS

command, permission = create_command(
    "spawn",
    "Warps you to the spawn!",
    ["/spawn"],
    ["primebds.command.spawn"]
)

spawn_cooldowns: dict[str, float] = {}
spawn_delays: dict[str, bool] = {}
spawn_tasks: dict[str, int] = {}

def handler(self: "PrimeBDS", sender: CommandSender, args: list[str]) -> bool:
    if not isinstance(sender, Player):
        sender.send_error_message("This command can only be executed by a player")
        return False

    spawn = self.serverdb.get_spawn(self.server)
    if not spawn:
        sender.send_message("§cNo spawn has been set yet")
        return False

    pos = spawn["pos"]
    if not pos:
        sender.send_message("§cSpawn position data is invalid")
        return False

    delay = spawn.get("delay", 0)
    cooldown_time = spawn.get("cooldown", 0)
    current_time = time()
    last_used = spawn_cooldowns.get(sender.id, 0)

    if spawn_delays.get(sender.id, False):
        sender.send_message("§cYou are already teleporting to spawn")
        return False

    if current_time - last_used < cooldown_time:
        remaining = cooldown_time - (current_time - last_used)
        sender.send_message(f"§cYou must wait {remaining:.2f}s before using /spawn again")
        return False

    if delay > 0:
        spawn_delays[sender.id] = True
        sender.send_popup(f"§aWarping to spawn in §e{delay:.1f}s")
        start_pos = sender.location
        start_time = time()

        def repeated_check():
            if sender.location.distance(start_pos) > 0.25:
                sender.send_message("§cTeleport cancelled because you moved!")
                spawn_delays[sender.id] = False
                task_id = spawn_tasks.pop(sender.id, None)
                if task_id:
                    self.server.scheduler.cancel_task(task_id)
                return True
            
            remaining = max(0, delay - (time() - start_time))
            sender.send_popup(f"§aWarping to spawn in §e{remaining:.1f}s")

            if time() - start_time >= delay:
                sender.teleport(pos)
                sender.send_message("§aYou have been warped to spawn!")
                spawn_cooldowns[sender.id] = time()
                spawn_delays[sender.id] = False
                task_id = spawn_tasks.pop(sender.id, None)
                if task_id:
                    self.server.scheduler.cancel_task(task_id)
                return True
            return False

        task = self.server.scheduler.run_task(self, repeated_check, delay=0, period=20)
        spawn_tasks[sender.id] = task.task_id

    else:
        sender.teleport(pos)
        sender.send_message("§aYou have been warped to spawn!")
        spawn_cooldowns[sender.id] = time()

    return True