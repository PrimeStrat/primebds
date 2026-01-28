from endstone.command import CommandSender
try:
    from endstone.command import BlockCommandSender
except ImportError:
    BlockCommandSender = None 
from endstone_primebds.utils.command_util import create_command
from endstone import Player
from endstone.util import Vector

from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from endstone_primebds.primebds import PrimeBDS

# Register command
command, permission = create_command(
    "jails",
    "Manages server jails!",
    [
        "/jails (list)[list_jails: list_jails]",
        "/jails (create|delete|tp)<jail_action: jail_action> <jail: string> [location: pos]"
    ],
    ["primebds.command.jails"]
)

def handler(self: "PrimeBDS", sender: CommandSender, args: list[str]) -> bool:
    if isinstance(sender, BlockCommandSender):
        sender.send_message("§cThis command cannot be automated")
        return False

    if any("@" in arg for arg in args):
        sender.send_message("§cTarget selectors are invalid for this command")
        return False

    if len(args) == 0 or args[0].lower() == "list":
        jails = self.serverdb.get_all_jails(self.server)
        if jails:
            sender.send_message("§6Jails on the server:")
            for jail_name in jails:
                sender.send_message(f"  §7- §e{jail_name}")
        else:
            sender.send_message("§cNo jails found")
        return True

    if len(args) >= 2:
        action = args[0].lower()
        jail_name = args[1]

        if action == "create":
            loc = args[2] if len(args) >= 3 else getattr(sender, "location", None)
            
            if not isinstance(loc, Vector):
                sender.send_message("§cCannot determine a valid location to create the jail")
                return False

            self.serverdb.create_jail(jail_name, loc)
            sender.send_message(f"§6Jail §e'{jail_name}' §6has been created at §e{round(loc.x)}, {round(loc.y)}, {round(loc.z)}")
            return True

        elif action == "delete":
            if self.serverdb.delete_jail(jail_name):
                sender.send_message(f"§6Jail §e'{jail_name}' §6has been deleted")
            else:
                sender.send_message(f"§cJail '{jail_name}' not found")
            return True

        elif action == "tp":
            jail = self.serverdb.get_jail(jail_name, self.server)
            if jail:
                if isinstance(sender, Player):
                    sender.teleport(jail["pos"])
                    sender.send_message(f"Teleported to jail '{jail['name']}'")
                else:
                    sender.send_message("§cOnly players can be teleported")
            else:
                sender.send_message(f"§cJail '{jail_name}' not found.")
            return True

    return False
