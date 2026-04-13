from endstone import Player
from endstone.command import CommandSender
from endstone_primebds.utils.command_util import create_command
from endstone_primebds.utils.target_selector_util import get_matching_actors
from endstone_primebds.utils.config_util import load_config

from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from endstone_primebds.primebds import PrimeBDS

import endstone_primebds.utils.internal_permissions_util as perms_util
config = load_config()
rank_meta_nametags = config["modules"]["server_messages"]["rank_meta_nametags"] 

# Register command
command, permission = create_command(
    "nickname",
    "Sets a display nickname!",
    ["/nickname [nick: string] [player: player]", "/nickname (clear)[clear_nick: clear_nick] [player: player]"],
    ["primebds.command.nickname"],
    "op",
    ["nick"]
)

# NICK COMMAND FUNCTIONALITY
def handler(self: "PrimeBDS", sender: CommandSender, args: list[str]) -> bool:
    if not isinstance(sender, Player):
        sender.send_error_message("This command can only be executed by a player.")
        return False

    targets = [self.server.get_player(sender.name)]

    if len(args) > 1:
        possible_target_arg = args[-1]
        matched_players = get_matching_actors(self, possible_target_arg, sender)
        if matched_players:
            targets = matched_players
            nick_arg_index = len(args) - 1  
            new_nick = " ".join(args[:nick_arg_index]).strip()
            if not new_nick:
                sender.send_message(f"{targets[0].name}'s current nickname is: §e{targets[0].name_tag}")
                return True
        else:
            new_nick = " ".join(args).strip()
    elif len(args) == 1:
        new_nick = args[0]
    else:
        sender.send_message(f"Your current nickname is: §e{targets[0].name_tag}")
        return True

    for target in targets:
        if new_nick.lower() == "clear":
            if rank_meta_nametags:
                user = self.db.get_online_user(target.xuid)
                prefix = perms_util.get_prefix(user.internal_rank, perms_util.PERMISSIONS)
                suffix = perms_util.get_suffix(user.internal_rank, perms_util.PERMISSIONS)
                target.name_tag = prefix+target.name+suffix
            else:
                target.name_tag = target.name 
        else:
            if new_nick:
                target.name_tag = new_nick 
            else:
                sender.send_error_message("Nickname cannot be empty.")
                return False
            
    if new_nick.lower() == "clear":
        if len(targets) == 1:
            sender.send_message(f"§e{targets[0].name}'s §rnickname has been reset")
        else:
            sender.send_message(f"§e{len(targets)} §rplayers had their nickname reset")
    else:
        if len(targets) == 1:
            sender.send_message(f"§e{targets[0].name}'s §rnickname was set to: §e{new_nick}")
        else:
            sender.send_message(f"§e{len(targets)} §rplayers had their nickname set to \"§{new_nick}\"")

    return True
