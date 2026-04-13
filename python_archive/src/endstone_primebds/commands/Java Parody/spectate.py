import random
from endstone import Player, GameMode
from endstone_primebds.utils.command_util import create_command
from endstone.command import CommandSender
from typing import TYPE_CHECKING
from endstone_primebds.utils.form_wrapper_util import (
    ActionFormData,
    ActionFormResponse,
)

from endstone_primebds.utils.target_selector_util import get_matching_actors
from endstone_primebds.utils.config_util import load_config

if TYPE_CHECKING:
    from endstone_primebds.primebds import PrimeBDS

# Register command
command, permission = create_command(
    "spectate",
    "Warps you to a non-spectating player!",
    ["/spectate [player: player]"],
    ["primebds.command.spectate"]
)

# SPECTATE COMMAND FUNCTIONALITY
def handler(self: "PrimeBDS", sender: CommandSender, args: list[str]) -> bool:

    if any("@a" in arg for arg in args):
        sender.send_message(f"§cYou cannot select all players for this command")
        return False

    config = load_config()
    check_gamemode = config["modules"]["spectator_check"].get("check_gamemode", True)
    force_gamemode = config["modules"]["spectator_check"].get("force_gamemode", True)
    check_tags = config["modules"]["spectator_check"].get("check_tags", False)
    dead_tags = config["modules"]["spectator_check"].get("allow_tags", [])
    ignore_tags = config["modules"]["spectator_check"].get("ignore_tags", [])

    if not isinstance(sender, Player):
        sender.send_error_message("This command can only be executed by a player.")
        return False

    if check_gamemode and sender.game_mode != GameMode.SPECTATOR:
        sender.send_message(f"You are not currently a spectator!")
        return True
    if check_tags and not any(tag in sender.scoreboard_tags for tag in dead_tags):
        sender.send_message(f"You are not currently a spectator!")
        return True

    def is_valid_spectate_target(player: Player) -> bool:
        """Returns True if the player is a valid spectate target based on config settings."""
        if check_gamemode and player.game_mode == GameMode.SPECTATOR:
            return False
        if check_tags:
            if any(tag in player.scoreboard_tags for tag in dead_tags):
                return False
            if any(tag in player.scoreboard_tags for tag in ignore_tags):
                return False
        return True

    if len(args) == 0:
        players_to_spectate = [p for p in self.server.online_players if is_valid_spectate_target(p)]
        if players_to_spectate:
            form = ActionFormData()
            form.title("Spectate Menu")
            form.body("Select a player to spectate!")

            for player in players_to_spectate:
                form.button(player.name_tag)

            def submit(player: Player, result: ActionFormResponse):
                if not result.canceled:
                    try:
                        selected_index = int(result.selection)
                        if 0 <= selected_index < len(players_to_spectate):
                            warp_player(player, players_to_spectate[selected_index], force_gamemode)
                        else:
                            player.send_message(f"Invalid selection.")
                    except ValueError:
                        player.send_message(f"Invalid selection index.")

            form.show(sender).then(lambda player=sender, result=ActionFormResponse: submit(player, result))
        else:
            sender.send_message(f"No players available to spectate.")
        return True
    else:
        targets = get_matching_actors(sender, args[0], sender)
        if not targets:
            sender.send_message("Unable to find target player")
            return False

        tried = set()
        random.shuffle(targets)
        for target in targets:
            if target is None or not is_valid_spectate_target(target) or target.name in tried:
                tried.add(target.name)
                continue

            warp_player(sender, target, force_gamemode)
            return True

        sender.send_message("No valid players available to spectate.")
        return False

def warp_player(sender: Player, target: Player, force_gamemode):
    """Warp the sender to the target player."""
    if force_gamemode:
        sender.game_mode = GameMode.SPECTATOR
    sender.teleport(target.location)
    sender.send_message(f"Now spectating {target.name_tag}")
