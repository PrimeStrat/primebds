from endstone import Player
from endstone.command import CommandSender
from endstone_primebds.utils.command_util import create_command
from time import time
from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from endstone_primebds.primebds import PrimeBDS

command, permission = create_command(
    "homeother",
    "Manage another player's homes!",
    [
        "/homeother <player: player>", 
        "/homeother <player: player> (list)[home_list_other: home_list_other]",
        "/homeother <player: player> (warp)[home_warp_other: home_warp_other] [name: str]",
        "/homeother <player: player> (set)[home_set_other: home_set_other] [name: str]",
        "/homeother <player: player> (delete)[home_del_other: home_del_other] [name: str]",
        "/homeother <player: player> (max)[home_max_other: home_max_other]"
    ],
    ["primebds.command.homeother"]
)

homeother_cooldowns: dict[str, float] = {}
homeother_delays: dict[str, bool] = {}

def handler(self: "PrimeBDS", sender: CommandSender, args: list[str]) -> bool:
    if not isinstance(sender, Player):
        sender.send_error_message("This command can only be executed by a player")
        return False

    if not args:
        sender.send_message("§cUsage: /homeother <player> [list/warp/set/del/max]")
        return False

    target_name = args[0]

    if any("@" in arg for arg in args):
        sender.send_message(f"§cTarget selectors are invalid for this command")
        return False
    else:
        target = self.server.get_player(target_name)
        if not target:
            sender.send_message(f"§cPlayer §e{target_name} §cnot found")
            return False

    target_xuid = target.xuid
    target_username = target.name

    sub = args[1].lower() if len(args) >= 2 else None

    exempt = sender.has_permission("primebds.exempt.homes.other")
    current_time = time()
    last_used = homeother_cooldowns.get(sender.id, 0)

    global_settings = self.serverdb.get_home_settings()
    delay = 0 if exempt else global_settings.get("delay", 0)
    cooldown = 0 if exempt else global_settings.get("cooldown", 0)

    if not exempt and current_time - last_used < cooldown:
        remaining = cooldown - (current_time - last_used)
        sender.send_message(f"§cYou must wait {remaining:.2f}s before using /homeother again")
        return False

    homes = self.serverdb.get_all_homes(self.server, username=target_username, xuid=target_xuid)
    
    if sub is None:
        if not homes:
            sender.send_message(f"§c{target_username} has no homes")
            return False
        default_home = list(homes.keys())[0]
        pos = homes[default_home]["pos"]
        sender.teleport(pos)
        sender.send_message(f"§aWarped to §e{target_username}'s {default_home}")
        homeother_cooldowns[sender.id] = current_time
        return True

    if sub == "list":
        if not homes:
            sender.send_message(f"§c{target_username} has no homes")
            return True
        msg_lines = [f"§7- §b{name}" for name in homes.keys()]
        sender.send_message(f"§a{target_username}'s homes:\n" + "\n".join(msg_lines))
        return True

    if sub == "warp" and len(args) >= 3:
        name = args[2]
        home = homes.get(name)
        if not home:
            sender.send_message(f"§cHome §e{name} §cdoes not exist for §e{target_username}")
            return True
        start_pos = sender.location
        start_time = time()

        if delay > 0:
            homeother_delays[sender.id] = True
            sender.send_popup(f"§aWarping to §e{target_username}'s {name} §7in {delay:.1f}s")

            def repeated_check():
                if sender.location.distance(start_pos) > 0.25:
                    sender.send_message("§cTeleport cancelled because you moved!")
                    homeother_delays[sender.id] = False
                    return True
                if time() - start_time >= delay:
                    sender.teleport(home["pos"])
                    sender.send_message(f"§aWarped to §e{target_username}'s {name}")
                    homeother_cooldowns[sender.id] = time()
                    homeother_delays[sender.id] = False
                    return True
                remaining = max(0, delay - (time() - start_time))
                sender.send_popup(f"§aWarping to §e{target_username}'s {name} §7in {remaining:.1f}s")
                return False

            self.server.scheduler.run_task(self, repeated_check, delay=0, period=20)
        else:
            sender.teleport(home["pos"])
            sender.send_message(f"§aWarped to §e{target_username}'s {name}")
            homeother_cooldowns[sender.id] = current_time
        return True

    if sub == "del" and len(args) >= 3:
        name = args[2]
        if self.serverdb.delete_home(name, username=target_username, xuid=target_xuid):
            sender.send_message(f"§aDeleted §e{target_username}'s {name}")
        else:
            sender.send_message(f"§c{target_username} does not have a home named §e{name}")
        return True

    if sub == "set":
        name = args[2] if len(args) >= 3 else "Home"
        max_homes = 1
        for perm in target.effective_permissions:
            if perm.permission.startswith("primebds.homes.") and perm.permission != "primebds.exempt.homes":
                try:
                    max_homes = max(max_homes, int(perm.permission.split(".")[-1]))
                except ValueError:
                    continue
        homes_count = len(homes)
        if homes_count >= max_homes:
            sender.send_message(f"§c{target_username} can only have {max_homes} homes")
            return True

        loc = sender.location
        if self.serverdb.create_home(target_xuid, target_username, name, loc):
            sender.send_message(f"§aSet §e{name} §afor {target_username}")
        else:
            sender.send_message(f"§c{target_username} already has a home named §e{name}")
        return True

    if sub == "max":
        if sender.has_permission("primebds.homes.exempt"):
            max_homes = "unlimited"
        else:
            max_homes = 1
            for permission in target.effective_permissions:
                perm = permission.permission
                if perm.startswith("primebds.homes.") and perm != "primebds.exempt.homes":
                    try:
                        max_homes = max(max_homes, int(perm.split(".")[-1]))
                    except ValueError:
                        continue
        sender.send_message(f"§e{target_username} §acan have up to §e{max_homes} §ahomes")
        return True

    sender.send_message("§cInvalid usage. /homeother <player> [list/warp/set/del/max]")
    return False
