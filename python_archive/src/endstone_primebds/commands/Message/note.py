from endstone.command import CommandSender
try:
    from endstone.command import BlockCommandSender
except ImportError:
    BlockCommandSender = None 
from endstone_primebds.utils.command_util import create_command
from endstone_primebds.utils.time_util import TimezoneUtils

from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from endstone_primebds.primebds import PrimeBDS

# Register command
command, permission = create_command(
    "note",
    "Manage mod notes on players!",
    ["/note <player: player> (clear)<note_clear: note_clear>",
     "/note <player: player> [page: int]",
     "/note <player: player> (remove)<note_remove: note_remove> <id: int>",
     "/note <player: player> (add)<note_add: note_add> <message: message>"],
    ["primebds.command.note"]
)

def handler(self: "PrimeBDS", sender: CommandSender, args: list[str]) -> bool:
    if BlockCommandSender is not None and isinstance(sender, BlockCommandSender):
       sender.send_message("§cThis command cannot be automated")
       return False



    if not args:
        sender.send_message("§cUsage: /note <player> (check|clear|remove|add) <...>")
        return False

    player = args[0]
    subcommand = args[1] if len(args) > 1 else 0

    if isinstance(subcommand, str) and subcommand.isdigit():
        subcommand = int(subcommand)

    xuid = self.db.get_xuid_by_name(player)
    name = player if not xuid else None 

    if not xuid and not name:
        sender.send_message("§cPlayer not found")
        return False
    
    if isinstance(subcommand, int):
        page = subcommand
        notes = self.db.get_notes(xuid=xuid, name=name)
        if not notes:
            sender.send_message(f"§6No notes found for §e{player}")
            return True

        notes_per_page = 5
        total_pages = (len(notes) + notes_per_page - 1) // notes_per_page

        # Clamp page to valid range
        if page < 1:
            page = 1
        if page > total_pages:
            sender.send_message(f"§cPage {page} is out of range. Showing last page {total_pages}")
            page = total_pages

        start = (page - 1) * notes_per_page
        end = start + notes_per_page
        page_notes = notes[start:end]

        sender.send_message(f"§6Notes for §e{player} §7(Page {page}/{total_pages}):")
        for note in page_notes:
            sender.send_message(
                f"§8[§7ID {note.id}§8] §f\"{note.note}\" §7- §e{note.added_by} §8[§c{TimezoneUtils.convert_to_timezone(note.timestamp, 'EST')}§8]"
            )
        return True

    elif subcommand == "clear":
        if not xuid:
            xuid = self.db.get_xuid_by_name(name)
        if not xuid:
            sender.send_message("§cPlayer not found")
            return False
        self.db.clear_notes(xuid)
        sender.send_message(f"§6Cleared all notes for §e{player}")

    elif subcommand == "remove":
        if len(args) < 3:
            sender.send_message("§cUsage: /note <player> remove <note_id>")
            return False
        try:
            note_id = int(args[2])
            if self.db.remove_note_by_id(note_id):
                sender.send_message(f"§6Removed note §eID {note_id}")
            else:
                sender.send_message("§cNote ID not found")
        except ValueError:
            sender.send_message("§cInvalid note ID")

    elif subcommand == "add":
        message_start = 2 if subcommand == "add" else 1
        if len(args) <= message_start:
            sender.send_message("§cPlease provide a message to add")
            return False
        note_message = " ".join(args[message_start:])
        self.db.add_note(note_message, sender.name, xuid=xuid, name=name)
        sender.send_message(f"§6Note added for §e{player}")

    else:
        sender.send_message("§cUnknown subcommand")
        return False

    return True
