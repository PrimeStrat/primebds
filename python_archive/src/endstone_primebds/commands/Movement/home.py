from endstone import Player
from endstone.command import CommandSender
from endstone_primebds.utils.command_util import create_command
from time import time
from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from endstone_primebds.primebds import PrimeBDS

command, permission = create_command(
    "home",
    "Manage and warp to homes!",
    [
        "/home",
        "/home (max)[home_max: home_max]",
        "/home (set)[home_set: home_set] [name: str]", 
        "/home (list)[home_list: home_list]",  
        "/home (warp)[home_warp: home_warp] <name: str>", 
        "/home (delete)[home_del: home_del] <name: str>"
    ],
    ["primebds.command.home"]
)

home_cooldowns: dict[str, float] = {}
home_delays: dict[str, bool] = {}

def handler(self: "PrimeBDS", sender: CommandSender, args: list[str]) -> bool:
    if not isinstance(sender, Player):
        sender.send_error_message("This command can only be executed by a player")
        return False

    settings = self.serverdb.get_home_settings()
    global_delay = settings.get("delay", 0)
    global_cooldown = settings.get("cooldown", 0)
    global_cost = settings.get("cost", 0)

    exempt_delay = sender.has_permission("primebds.exempt.home.delays")
    exempt_cooldown = sender.has_permission("primebds.exempt.home.cooldowns")

    current_time = time()
    last_used = home_cooldowns.get(sender.id, 0)

    if not exempt_cooldown and current_time - last_used < global_cooldown:
        remaining = global_cooldown - (current_time - last_used)
        sender.send_message(f"§cYou must wait {remaining:.2f}s before using /home again")
        return False

    if not args:
        homes = self.serverdb.get_all_homes(self.server, username=sender.name, xuid=sender.xuid)
        if not homes:
            sender.send_message("§cYou have no homes set")
            return False
        default_home = list(homes.keys())[0]
        pos = homes[default_home]["pos"]
        sender.teleport(pos)
        sender.send_message(f"§aWarped to §e{default_home}")
        home_cooldowns[sender.id] = current_time
        return True

    sub = args[0].lower()

    if sub == "max":
        if sender.has_permission("primebds.homes.exempt"):
            max_homes = "unlimited"
        else:
            max_homes = 1 
            for permission in sender.effective_permissions:
                perm = permission.permission
                if perm.startswith("primebds.homes.") and perm != "primebds.homes.exempt":
                    try:
                        max_homes = max(max_homes, int(perm.split(".")[-1]))
                    except ValueError:
                        continue

        sender.send_message(f"§aYou can have up to §e{max_homes} §ahomes")
        return True

    if sub == "set":
        name = args[1] if len(args) >= 2 else "Home"

        if sender.has_permission("primebds.exempt.homes"):
            max_homes = float("inf")
        else:
            max_homes = 1  # default
            for perm in sender.effective_permissions:
                if perm.startswith("primebds.homes.") and perm != "primebds.exempt.homes":
                    try:
                        max_homes = max(max_homes, int(perm.split(".")[-1]))
                    except ValueError:
                        continue

        homes = self.serverdb.get_all_homes(self.server, username=sender.name, xuid=sender.xuid)
        if len(homes) >= max_homes:
            sender.send_message(f"§cYou can only have {max_homes} homes")
            return True

        loc = sender.location
        if self.serverdb.create_home(sender.xuid, sender.name, name, loc):
            sender.send_message(f"§e{name} §aset successfully at your current location")
        else:
            sender.send_message(f"§e{name} §calready exists")
        return True

    if sub == "list":
        homes = self.serverdb.get_all_homes(self.server, username=sender.name, xuid=sender.xuid)
        if not homes:
            sender.send_message("§cYou have no homes set")
            return True

        msg_lines = [f"§7- §b{name}" for name in homes.keys()]
        sender.send_message("§aYour homes:\n" + "\n".join(msg_lines))
        return True

    if sub == "warp" and len(args) >= 2:
        name = args[1]
        home = self.serverdb.get_home(name, self.server, username=sender.name, xuid=sender.xuid)
        if not home or not home["pos"]:
            sender.send_message(f"§e{name} §cdoes not exist")
            return True

        delay = 0 if exempt_delay else global_delay
        start_pos = sender.location
        start_time = time()

        if delay > 0:
            home_delays[sender.id] = True
            sender.send_popup(f"§aWarping to §e{name} §ain {delay:.1f}s")

            def repeated_check():
                if sender.location.distance(start_pos) > 0.25:
                    sender.send_message("§cTeleport cancelled because you moved!")
                    home_delays[sender.id] = False
                    return True
                if time() - start_time >= delay:
                    sender.teleport(home["pos"])
                    sender.send_message(f"§aWarped to §e{name}")
                    home_cooldowns[sender.id] = time()
                    home_delays[sender.id] = False
                    return True
                remaining = max(0, delay - (time() - start_time))
                sender.send_popup(f"§aWarping to §e{name} §ain {remaining:.1f}s")
                return False

            self.server.scheduler.run_task(self, repeated_check, delay=0, period=20)
        else:
            sender.teleport(home["pos"])
            sender.send_message(f"§aWarped to §e{name}")
            home_cooldowns[sender.id] = current_time
        return True

    if sub in ("del", "delete") and len(args) >= 2:
        name = args[1]
        if self.serverdb.delete_home(name, username=sender.name, xuid=sender.xuid):
            sender.send_message(f"§e{name} §adeleted")
        else:
            sender.send_message(f"§e{name} §cdoes not exist")
        return True

    sender.send_message("§cInvalid usage. /home [set/list/warp/del]")
    return False
