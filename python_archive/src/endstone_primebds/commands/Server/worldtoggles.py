from endstone import Player
from endstone_primebds.utils.form_wrapper_util import (
    ActionFormData,
    ActionFormResponse,
)
from endstone.command import CommandSender
from endstone_primebds.utils.command_util import create_command
from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from endstone_primebds.primebds import PrimeBDS

# Register command
command, permission = create_command(
    "worldtoggles",
    "Toggles internal gamerules handled by PrimeBDS through Endstone!",
    ["/worldtoggles (can_interact|can_emote|can_decay_leaves|can_change_skin|can_pickup_items|can_sleep|can_split_screen)[internal_gamerule: internal_gamerule] [toggle: bool]"],
    ["primebds.command.worldtoggles"]
)

def handler(self: "PrimeBDS", sender: CommandSender, args: list[str]) -> bool:
    if not isinstance(sender, Player):
        sender.send_message("§cOnly players can use this command")
        return True

    if not args:
        show_gamerule_form(self, sender)
        return True

    key = args[0].lower()
    if key not in self.gamerules:
        sender.send_message(f"§cUnknown gamerule: {key}")
        return True

    if len(args) < 2 or args[1].lower() not in ("true", "false", "1", "0"):
        sender.send_message("§cUsage: /worldtoggles <gamerule> <true|false>")
        return True

    value = args[1].lower() in ("true", "1")
    self.gamerules[key] = int(value)

    self.serverdb.execute(
        f"UPDATE server_info SET {key} = ? WHERE id = 1",
        (self.gamerules[key],)
    )

    sender.send_message(f"§eGamerule {key.replace('_', ' ').title()} set to {'§aON' if value else '§cOFF'}")
    return True

def show_gamerule_form(self: "PrimeBDS", player):
    buttons = []

    keys = list(self.gamerules.keys())
    for key in keys:
        value = self.gamerules[key]
        state = "§aON§r" if value else "§cOFF§r"
        label = key.replace("_", " ").title()
        buttons.append(f"§r{label}: {state}")

    form = ActionFormData()
    form.title("World Gamerule Toggles")
    for button in buttons:
        form.button(button)
    form.button("Close")

    def submit(player: Player, result: ActionFormResponse):
        if not result or result.selection is None:
            return True

        if result.selection >= len(buttons):
            return True
        
        clicked_key = keys[result.selection]
        self.gamerules[clicked_key] ^= 1

        self.serverdb.execute(
            f"UPDATE server_info SET {clicked_key} = ? WHERE id = 1",
            (self.gamerules[clicked_key],)
        )

        show_gamerule_form(self, player)

    form.show(player).then(lambda player=player, result=ActionFormResponse: submit(player, result))
