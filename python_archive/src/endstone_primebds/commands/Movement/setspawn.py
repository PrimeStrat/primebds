from endstone import Player
from endstone.command import CommandSender
from endstone_primebds.utils.command_util import create_command
from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from endstone_primebds.primebds import PrimeBDS

# Register command
command, permission = create_command(
    "setspawn",
    "Sets the global spawn point!",
    ["/setspawn [delay: int] [cooldown: int] [cost: int]"],
    ["primebds.command.setspawn"]
)

def handler(self: "PrimeBDS", sender: CommandSender, args: list[str]) -> bool:
    if not isinstance(sender, Player):
        sender.send_error_message("§cOnly players can use this command.")
        return False

    delay = float(args[0]) if len(args) >= 1 and args[0].replace('.', '', 1).isdigit() else 0.0
    cooldown = float(args[1]) if len(args) >= 2 and args[1].replace('.', '', 1).isdigit() else 0.0
    cost = float(args[2]) if len(args) >= 3 and args[2].replace('.', '', 1).isdigit() else 0.0

    location = sender.location
    existing = self.serverdb.get_spawn(self.server)

    if existing:
        self.serverdb.update_spawn_property("pos", location)
        self.serverdb.update_spawn_property("delay", delay)
        self.serverdb.update_spawn_property("cooldown", cooldown)
        self.serverdb.update_spawn_property("cost", cost)
    else:
        self.serverdb.create_spawn(location, cost=cost, delay=delay, cooldown=cooldown)

    sender.send_message(f"§aSpawn point created successfully at §e{location.x:.2f} {location.y:.2f} {location.z:.2f} §7(§e{location.dimension.name}§7)")

    return True
