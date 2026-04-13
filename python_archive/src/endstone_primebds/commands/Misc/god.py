from endstone import Player
from endstone.command import CommandSender
from endstone_primebds.utils.command_util import create_command
from endstone_primebds.utils.target_selector_util import get_matching_actors

from typing import TYPE_CHECKING
if TYPE_CHECKING:
    from endstone_primebds.primebds import PrimeBDS

# Register command
command, permission = create_command(
    "god",
    "Toggles or sets invulnerability for yourself or other players.",
    ["/god [player: player] [toggle: bool]"],
    ["primebds.command.god", "primebds.command.god.other"],
    "op"
)

def handler(self: "PrimeBDS", sender: CommandSender, args: list[str]) -> bool:
    if len(args) == 0:
        if not isinstance(sender, Player):
            sender.send_message("§cThis command can only be executed by a player")
            return False
        
        if sender.id in self.isgod:
            self.isgod.remove(sender.id)
            sender.send_message("§cYou are no longer invulnerable")
        else:
            self.isgod.add(sender.id)
            sender.send_message("§aYou are now invulnerable")
        return True

    if not sender.has_permission("primebds.command.god.other"):
        sender.send_message("§cYou do not have permission to modify others' invulnerability")
        return True

    selector = args[0]
    force_arg = args[1].lower() if len(args) > 1 else None
    targets = get_matching_actors(self, selector, sender)

    if not targets:
        sender.send_message(f"§cNo matching players found for '{selector}'.")
        return True

    toggled_on = []
    toggled_off = []

    for target in targets:
        if force_arg in ("true", "on", "1"):
            should_enable = True
        elif force_arg in ("false", "off", "0"):
            should_enable = False
        else:
            # Toggle if not explicitly set
            should_enable = target.id not in self.isgod

        # Apply change
        if should_enable:
            self.isgod.add(target.id)
            toggled_on.append(target)
            target.send_message("§aYou are now invulnerable")
        else:
            self.isgod.discard(target.id)
            toggled_off.append(target)
            target.send_message("§cYou are no longer invulnerable")

    # --- FEEDBACK ---
    if len(targets) == 1:
        t = targets[0]
        state = "invulnerable" if t in toggled_on else "no longer invulnerable"
        sender.send_message(f"§e{t.name} §ris now {state}")
    else:
        sender.send_message(
            f"§a{len(toggled_on)} enabled, §c{len(toggled_off)} disabled invulnerability"
        )

    return True
