from endstone import Player
from endstone.command import CommandSender
from endstone_primebds.utils.command_util import create_command
from time import time
from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from endstone_primebds.primebds import PrimeBDS

command, permission = create_command(
    "back",
    "Warps you to your last saved location!",
    ["/back"],
    ["primebds.command.back"]
)

back_cooldowns: dict[str, float] = {}
back_delays: dict[str, bool] = {}
back_tasks: dict[str, int] = {}

def handler(self: "PrimeBDS", sender: CommandSender, args: list[str]) -> bool:
    if not isinstance(sender, Player):
        sender.send_error_message("This command can only be executed by a player")
        return False

    last_warp = self.serverdb.get_last_warp(self.server, xuid=sender.xuid, username=sender.name)
    if not last_warp:
        sender.send_message("§cNo last location saved to return to")
        return False

    pos = last_warp.get("pos")
    if not pos:
        sender.send_message("§cLast location data is invalid")
        return False

    delay = last_warp.get("delay", 0)
    cooldown_time = last_warp.get("cooldown", 0)
    current_time = time()
    last_used = back_cooldowns.get(sender.id, 0)

    if back_delays.get(sender.id, False):
        sender.send_message("§cYou are already teleporting to your last location")
        return False

    if current_time - last_used < cooldown_time:
        remaining = cooldown_time - (current_time - last_used)
        sender.send_message(f"§cYou must wait {remaining:.2f}s before using /back again")
        return False

    if delay > 0:
        back_delays[sender.id] = True
        start_pos = sender.location
        start_time = time()

        def repeated_check():
            if sender.location.distance(start_pos) > 0.25:
                sender.send_message("§cTeleport cancelled because you moved!")
                back_delays[sender.id] = False
                task_id = back_tasks.pop(sender.id, None)
                if task_id:
                    self.server.scheduler.cancel_task(task_id)
                return True

            remaining = max(0, delay - (time() - start_time))
            sender.send_popup(f"§aWarping to last location in §e{remaining:.1f}s")

            if remaining <= 0:
                sender.teleport(pos)
                sender.send_message("§aYou have been warped to your last location!")
                back_cooldowns[sender.id] = time()
                back_delays[sender.id] = False
                task_id = back_tasks.pop(sender.id, None)
                if task_id:
                    self.server.scheduler.cancel_task(task_id)
                return True

            return False

        task = self.server.scheduler.run_task(self, repeated_check, delay=0, period=20)
        back_tasks[sender.id] = task.task_id

    else:
        sender.teleport(pos)
        sender.send_message("§aYou have been warped to your last location!")
        back_cooldowns[sender.id] = time()

    return True
