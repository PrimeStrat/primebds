from endstone import Player
from endstone.command import CommandSender
from endstone_primebds.utils.command_util import create_command
from endstone_primebds.utils.config_util import load_config

from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from endstone_primebds.primebds import PrimeBDS

# Register command
command, permission = create_command(
    "reply",
    "Reply to the user who last sent you a message!",
    ["/reply <message: message>"],
    ["primebds.command.reply"],
    "op",
    ["r"]
)

def handler(self: "PrimeBDS", sender: CommandSender, args: list[str]) -> bool:

    if not isinstance(sender, Player):
        sender.send_message("This command can only be executed by a player")
        return False

    player = self.server.get_player(sender.name)
    user = self.db.get_online_user(player.xuid)
    last_messaged = user.last_messaged

    if last_messaged:
        target = self.server.get_player(last_messaged)
        if target:
            config = load_config()
            if config["modules"]["server_messages"]["enhanced_whispers"]:
                sender.send_message(f"{config['modules']['server_messages']['whisper_prefix']}§7To {target.name}: §o{args[0]}")
                target.send_message(f"{config['modules']['server_messages']['whisper_prefix']}§7From {sender.name}: §o{args[0]}")
            else:
                sender.send_message(f"You whisper to {target.name}: {args[0]}")
                target.send_message(f"{player.name_tag} §7§o{player.name} whispers to you: {args[0]}")

            for pl in self.server.online_players:  
                otheruser = self.db.get_online_user(pl.xuid)
                if otheruser:
                    if otheruser.enabled_ss == 1 and pl.has_permission("primebds.command.socialspy"):
                        pl.send_message(f"{config['modules']['server_messages']['social_spy_prefix']}§8[§r{player.name} §7-> §r{target.name}§8] §7{args[0]}")

        else:
            sender.send_message(f"§c{last_messaged} is not online")
    else:
        sender.send_message(f"§cYou cannot reply to no one")
