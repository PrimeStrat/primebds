from endstone import Player
from endstone.command import CommandSender
from endstone_primebds.utils.command_util import create_command
from endstone_primebds.utils.config_util import load_config
from endstone_primebds.handlers.intervals import start_afk_check_if_needed, stop_afk_check_if_not_needed

from typing import TYPE_CHECKING
if TYPE_CHECKING:
    from endstone_primebds.primebds import PrimeBDS

# Register command
command, permission = create_command(
    "afk",
    "Toggles AFK mode for yourself!",
    ["/afk"],
    ["primebds.command.afk"]
)

def handler(self: "PrimeBDS", sender: CommandSender, args: list[str]) -> bool:
    if not isinstance(sender, Player):
        sender.send_message("§cOnly players can use this command.")
        return True

    player = sender
    row = self.db.execute("SELECT is_afk FROM users WHERE xuid = ?", (player.xuid,)).fetchone()
    is_afk = bool(row[0]) if row else False

    if not is_afk:
        self.db.update_user_data(player.name, "is_afk", 1)
        player.send_message("§7You are now AFK")
        config = load_config()
        broadcast = config["modules"]["afk"]["broadcast_afk_status"]
        if broadcast:
            self.server.broadcast_message(f"§e{player.name} is now AFK")
        start_afk_check_if_needed(self)
    else:
        self.db.update_user_data(player.name, "is_afk", 0)
        player.send_message("§7You are no longer AFK")
        config = load_config()
        broadcast = config["modules"]["afk"]["broadcast_afk_status"]
        if broadcast:
            self.server.broadcast_message(f"§e{player.name} is no longer AFK")
        stop_afk_check_if_not_needed(self)

    return True
