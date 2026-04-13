from endstone_primebds.utils.time_util import TimezoneUtils

from endstone import ColorFormat, Player
from endstone.command import CommandSender
try:
    from endstone.command import BlockCommandSender
except ImportError:
    BlockCommandSender = None 
from endstone_primebds.utils.command_util import create_command
from typing import TYPE_CHECKING

from endstone_primebds.utils.form_wrapper_util import ActionFormResponse, ActionFormData

if TYPE_CHECKING:
    from endstone_primebds.primebds import PrimeBDS

# Register command
command, permission = create_command(
    "punishments",
    "Manage punishment history of a specified player!",
    ["/punishments <player: player> [page: int]",
     "/punishments <player: player> (remove|clear) <punishment_removal: remove_punishment_log>"],
    ["primebds.command.punishments"]
)

# PUNISHMENTS CMD FUNCTIONALITY
def handler(self: "PrimeBDS", sender: CommandSender, args: list[str]) -> bool:
    if BlockCommandSender is not None and isinstance(sender, BlockCommandSender):
       sender.send_message("§cThis command cannot be automated")
       return False



    if not args:
        sender.send_message(
            ColorFormat.red("Usage: /punishments <player> [page] OR /punishments <player> (remove|clear)"))
        return False
    
    if any("@" in arg for arg in args):
        sender.send_message(f"§cTarget selectors are invalid for this command")
        return False

    target_name = args[0]

    if len(args) > 1:
        action = args[1].lower()

        if action == "remove":
            return remove_punishment_by_id(self, sender, target_name)

        elif action == "clear":
            return clear_all_punishments(self, sender, target_name)

    # Retrieve punishment history
    page = int(args[1]) if len(args) > 1 and args[1].isdigit() else 1

    if page < 1:
        sender.send_message(f"Page number must be 1 or higher")
        return False

    history_message = self.db.print_punishment_history(target_name, page)

    if not history_message:
        sender.send_message(f"No punishment history found for §e{target_name}")
        return True

    sender.send_message(history_message)
    return True

def clear_all_punishments(self: "PrimeBDS", sender: CommandSender, target_name: str) -> bool:
    """Clears all punishment logs for the selected player."""
    

    success = self.db.delete_all_punishment_logs_by_name(target_name)

    if success:
        sender.send_message(f"Successfully cleared all punishments for §e{target_name}")
    else:
        sender.send_message(f"Failed to clear punishments for §e{target_name}")

    return True

def remove_punishment_by_id(self: "PrimeBDS", sender: CommandSender, target_name: str) -> bool:
    """Removes a specific punishment by ID using a menu."""

    # Retrieve punishment history
    
    history = self.db.print_punishment_history(target_name)
    punish_log = self.db.get_punishment_logs(target_name)

    if not history:
        sender.send_message(f"No more punishments found for §e{target_name}.")
        return False

    # Create action form with punishments listed as buttons
    form = ActionFormData()
    form.title("Punishment Removal")
    form.button("Cancel")

    for punishment in punish_log:
        punishment_text = (
            f"{punishment.action_type}: {punishment.reason}\n"
            f"{ColorFormat.DARK_GRAY}("
            f"§6{TimezoneUtils.convert_to_timezone(punishment.timestamp, 'EST')}"
            f"{ColorFormat.DARK_GRAY})"
        )
        form.button(f"§c{punishment_text}")

    # Handle the player's selection
    def submit(player: Player, result: ActionFormResponse):
        if result.canceled or result.selection == 0:
            return

        # Remove punishment by ID
        punishment_id = punish_log[int(result.selection)-1].id
        success = self.db.remove_punishment_log_by_id(target_name, punishment_id)

        if success:
            player.send_message(
                f"Successfully removed punishment ID {punishment_id} for §e{target_name}")
        else:
            player.send_message(
                f"Failed to remove punishment ID {punishment_id} for §e{target_name}")

        remove_punishment_by_id(self, sender, target_name)

    # Show the form and wait for the player's response
    form.show(sender).then(
        lambda result, player=sender: submit(result, player)
    )

    return True