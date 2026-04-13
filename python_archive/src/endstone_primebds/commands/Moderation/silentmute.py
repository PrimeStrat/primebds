from endstone.command import CommandSender
from endstone_primebds.utils.command_util import create_command
from endstone_primebds.utils.target_selector_util import get_matching_actors

from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from endstone_primebds.primebds import PrimeBDS

# Register command
command, permission = create_command(
    "silentmute",
    "Permanently mutes a player silently and expires on toggle or server restart!",
    ["/silentmute <player: player> [toggle: bool]"],
    ["primebds.command.silentmute"]
)

def handler(self: "PrimeBDS", sender: CommandSender, args: list[str]) -> bool:
    if len(args) < 1:
        sender.send_message(f"Usage: /silentmute <player> [toggle: bool]")
        return False

    selector = args[0].strip('"')
    matched = get_matching_actors(self, selector, sender)

    if not matched:
        sender.send_error_message(f"No matching players found for '{selector}'")
        return True

    force_state = None
    if len(args) >= 2:
        arg = args[1].lower()
        if arg in ["true", "1", "yes", "on"]:
            force_state = True
        elif arg in ["false", "0", "no", "off"]:
            force_state = False
        else:
            sender.send_error_message(f"Invalid toggle value '{args[1]}'. Use true/false")
            return False

    muted_count = 0
    unmuted_count = 0

    for player in matched:
        target_id = player.xuid if hasattr(player, "xuid") else player

        if force_state is None:
            if target_id in self.silentmutes:
                self.silentmutes.remove(target_id)
                unmuted_count += 1
            else:
                self.silentmutes.add(target_id)
                muted_count += 1
        elif force_state:
            if target_id not in self.silentmutes:
                self.silentmutes.add(target_id)
                muted_count += 1
        else:
            if target_id in self.silentmutes:
                self.silentmutes.remove(target_id)
                unmuted_count += 1

    if muted_count:
        sender.send_message(f"§e{muted_count} player{'s' if muted_count > 1 else ''} §6muted silently")
    if unmuted_count:
        sender.send_message(f"§e{unmuted_count} player{'s' if unmuted_count > 1 else ''} §6unmuted")

    return True

