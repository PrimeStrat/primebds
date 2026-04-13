from endstone import Player
from endstone.command import CommandSender
from endstone_primebds.utils.command_util import create_command
from endstone_primebds.utils.form_wrapper_util import (
    ModalFormData
)

from typing import TYPE_CHECKING
if TYPE_CHECKING:
    from endstone_primebds.primebds import PrimeBDS

# Register command
command, permission = create_command(
    "cords",
    "Print or copy your current position!",
    ["/cords (print)[print_cords: print_cords]"],
    ["primebds.command.cords"],
    "op",
    ["blockpos", "pos"]
)

def handler(self: "PrimeBDS", sender: CommandSender, args: list[str]) -> bool:
    if not isinstance(sender, Player):
        sender.send_message("§cOnly players can use this command.")
        return True

    player = self.server.get_player(sender.name)
    if player:
        if len(args) == 0:
            player.send_message(f"{player.location.block_x} {player.location.block_y} {player.location.block_z}")
        else:
            show_cords_form(player)

    return True

def show_cords_form(player):
    form = ModalFormData()
    form.title("Current Position")
    form.text_field(f"Current Block Position", f"{player.location.block_x} {player.location.block_y} {player.location.block_z}", f"{player.location.block_x} {player.location.block_y} {player.location.block_z}")
    form.show(player)