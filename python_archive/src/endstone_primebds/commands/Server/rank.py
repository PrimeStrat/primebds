import re
from endstone.command import CommandSender
try:
    from endstone.command import BlockCommandSender
except ImportError:
    BlockCommandSender = None 
from endstone_primebds.utils.command_util import create_command
import endstone_primebds.utils.internal_permissions_util as perms_util
from endstone_primebds.utils.config_util import load_permissions, save_permissions

from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from endstone_primebds.primebds import PrimeBDS

command, permission = create_command(
    "rank",
    "Sets the internal rank for a player!",
    [
        "/rank (set)<rank_set: rank_set> <player: player> <rank: string>",
        "/rank (prefix|suffix)<rank_meta: rank_meta> <rank: string> <meta: message>",
        "/rank (perm)<rank_perm: rank_perm> (add|remove)<perm_action: perm_action> <rank: string> <perm: string> [state: bool]",
        "/rank (weight)<rank_weight: rank_weight> <rank: string> <weight: int>",
        "/rank (inherit)<rank_inherit: rank_inherit> <rank_child: string> <rank_parent: string>",
        "/rank (create|delete|list|info)<rank_action: rank_action> [rank: message]"
    ],
    ["primebds.command.rank"]
)

# RANK COMMAND FUNCTIONALITY
def handler(self: "PrimeBDS", sender: CommandSender, args: list[str]) -> bool:
    if BlockCommandSender is not None and isinstance(sender, BlockCommandSender):
       sender.send_message("§cThis command cannot be automated")
       return False

    if any("@" in arg for arg in args):
        sender.send_message(f"§cTarget selectors are invalid for this command")
        return False

    target = args[1] if len(args) > 1 else None
    subaction = args[0].lower()
    
    perms = load_permissions()

    def find_rank(name: str, perms: dict) -> str | None:
        return next((r for r in perms if r.lower() == name.lower()), None)

    def rank_exists(name: str, perms: dict) -> bool:
        return find_rank(name, perms) is not None

    def permission_exists(rank: str, perm: str, perms: dict) -> bool:
        rank_key = find_rank(rank, perms)
        if not rank_key:
            return False
        return any(p.lower() == perm.lower() for p in perms[rank_key].get("permissions", {}))

    if subaction == "set":
        perms_util.reload_rank_list()

        user = self.db.get_offline_user(target)
        if not user:
            sender.send_message(f"§cPlayer \"{target}\" not found")
            return False
        
        new_rank_lower = args[2].lower()
        ranks_map = {r.lower(): r for r in perms_util.get_ranks()}

        if new_rank_lower == "op":
            new_rank_lower = "operator"

        if new_rank_lower not in ranks_map:
            sender.send_message(f"§cInvalid rank: §e{args[2]}§c. Valid ranks are: §e{', '.join(perms_util.get_ranks())}")
            return False

        proper_rank = ranks_map[new_rank_lower]

        if proper_rank == user.internal_rank:
            sender.send_message(f"§bPlayer §e{target} §balready is set to this rank")
            return False
        else:
            self.db.update_user_data(target, 'internal_rank', proper_rank)

            player = self.server.get_player(target)
            if player:
                if proper_rank.lower() == "operator":
                    self.server.dispatch_command(self.server.command_sender, f"op \"{target}\"")
                else:
                    self.server.dispatch_command(self.server.command_sender, f"deop \"{target}\"")
                    
                self.reload_custom_perms(player)
                perms_util.invalidate_perm_cache(player.xuid)
                
            sender.send_message(f"§bPlayer §e{target}'s §brank was updated to §e{proper_rank}")
            return True

    elif subaction == "create":
        rank_name = args[1]

        if rank_exists(rank_name, perms):
            sender.send_message(f"§cRank \"{rank_name}\" already exists")
            return False

        perms[rank_name] = {
            "permissions": {},
            "inherits": [],
            "weight": 0
        }
        save_permissions(perms, True)
        sender.send_message(f"§bCreated rank §e\"{rank_name}\" §bsuccessfully")
        perms_util.reload_rank_list()
        return True
    
    elif subaction == "delete":
        rank_name = args[1]

        if not rank_exists(rank_name, perms):
            sender.send_message(f"§cRank \"{rank_name}\" does not exist")
            return False

        actual_rank = find_rank(rank_name, perms)
        if actual_rank == "Default" or actual_rank == "Operator":
            sender.send_message(f"§cThis rank cannot be deleted")
            return False

        del perms[actual_rank]
        save_permissions(perms, True)
        updatePermissionsFiltered(self, {actual_rank})
        sender.send_message(f"§bDeleted rank §e\"{actual_rank}\" §bsuccessfully")
        perms_util.reload_rank_list()
        return True

    elif subaction == "info":
        if len(args) < 2:
            sender.send_message(f"§cYou must specify a rank")
            return False
        rank_name = args[1].lower()

        ranks_map = {r.lower(): r for r in perms_util.get_ranks()}

        if rank_name == "op":
            rank_name = "operator"

        if rank_name not in ranks_map:
            sender.send_message(f"§cInvalid rank: §e{args[1]}§c. Valid ranks are: §e{', '.join(perms_util.get_ranks())}")
            return False

        proper_rank = ranks_map[rank_name]

        rank = perms[proper_rank]
        sender.send_message(f"§bRank: §r{proper_rank}")
        sender.send_message(f"§bWeight: §r{rank.get('weight', 0)}")
        sender.send_message(f"§bInherits: §r{rank.get('inherits', [])}")
        sender.send_message(f"§bPrefix: §r{rank.get('prefix', "Unset")}")
        sender.send_message(f"§bSuffix: §r{rank.get('suffix', "Unset")}")
        sender.send_message("§bPermissions:")
        for perm, value in rank.get("permissions", {}).items():
            color = "§a" if value else "§c"
            sender.send_message(f"  §7- §e{perm}: {color}{value}")
        
        return True

    elif subaction == "perm":
        
        if args[1] == "add":
            rank_name = args[2]
            permission = args[3]
            state = True 
            if len(args) > 4:
                state = args[4].lower() in ("true", "1", "yes", "on")

            if rank_name == "op":
                rank_name = "operator"

            if not rank_exists(rank_name, perms):
                sender.send_message(f"§cRank \"{rank_name}\" does not exist")
                return False

            actual_rank = find_rank(rank_name, perms)
            perms[actual_rank]["permissions"][permission] = state
            save_permissions(perms, True)
            updatePermissionsFiltered(self, {actual_rank})
            sender.send_message(f"§bSet permission §e\"{permission}\" = {state} §bfor rank §e\"{actual_rank}\"")
            return True

        elif args[1] == "remove":
            rank_name = args[2]
            permission = args[3]

            if rank_name == "op":
                rank_name = "operator"

            if not rank_exists(rank_name, perms):
                sender.send_message(f"§cRank \"{rank_name}\" does not exist")
                return False

            actual_rank = find_rank(rank_name, perms)
            if not permission_exists(actual_rank, permission, perms):
                sender.send_message(f"Rank §e\"{actual_rank}\" §rdoes not have permission §e\"{permission}\"")
                return False

            permission = permission.lower()
            perms[actual_rank]["permissions"].pop(permission, None)
            save_permissions(perms, True)
            updatePermissionsFiltered(self, {actual_rank})
            sender.send_message(f"§bRemoved permission §e\"{permission}\" from rank §e\"{actual_rank}\"")
            return True

    elif subaction == "list":
        if not perms:
            sender.send_message("§eNo ranks available.")
            return True

        sender.send_message("§bAvailable ranks:")
        for rank in perms:
            sender.send_message(f" §7- §e{rank}")
        return True

    elif subaction == "inherit":
        child_rank = args[1]
        parent_rank = args[2]

        if child_rank == "op":
            child_rank = "operator"
        if parent_rank == "op":
            parent_rank = "operator"

        child = find_rank(child_rank, perms)
        parent = find_rank(parent_rank, perms)

        if not child:
            sender.send_message(f"§cChild rank \"{child_rank}\" does not exist")
            return False
        if not parent:
            sender.send_message(f"§cParent rank \"{parent_rank}\" does not exist")
            return False

        if parent in perms[child]["inherits"]:
            sender.send_message(f"§bRank §e\"{child}\" §balready inherits from §e\"{parent}\"")
            return False

        perms[child]["inherits"].append(parent)
        save_permissions(perms, True)
        updatePermissionsFiltered(self, {child})
        sender.send_message(f"§bRank §e\"{child}\" §bnow inherits from §e\"{parent}\"")
        return True

    elif subaction == "weight":
        rank_name = args[1].lower()
        if rank_name == "op":
            rank_name = "operator"

        if not rank_exists(rank_name, perms):
            sender.send_message(f"§cRank \"{rank_name}\" does not exist")
            return False

        actual_rank = find_rank(rank_name, perms)

        try:
            new_weight = int(args[2])
        except ValueError:
            sender.send_message("§cWeight must be an integer")
            return

        perms[actual_rank]["weight"] = new_weight
        save_permissions(perms, True)
        updatePermissionsFiltered(self, {actual_rank})
        sender.send_message(f"§bUpdated weight for rank §e{rank_name} §bto §e{new_weight}")

    elif subaction == "prefix":
        perms_util.clear_prefix_suffix_cache()
        rank_name = args[1].lower()
        if rank_name == "op":
            rank_name = "operator"

        if not rank_exists(rank_name, perms):
            sender.send_message(f"§cRank \"{rank_name}\" does not exist")
            return False

        actual_rank = find_rank(rank_name, perms)

        new_prefix = args[2]
        perms[actual_rank]["prefix"] = new_prefix
        save_permissions(perms, True)
        updatePermissionsFiltered(self, {actual_rank})
        sender.send_message(f"§bUpdated prefix for rank §e{rank_name} §bto §e{new_prefix}")

        return True
    
    elif subaction == "suffix":
        perms_util.clear_prefix_suffix_cache()
        rank_name = args[1].lower()
        if rank_name == "op":
            rank_name = "operator"
            
        if not rank_exists(rank_name, perms):
            sender.send_message(f"§cRank \"{rank_name}\" does not exist")
            return False

        actual_rank = find_rank(rank_name, perms)
        new_suffix = args[2]
        perms[actual_rank]["suffix"] = new_suffix
        save_permissions(perms, True)
        updatePermissionsFiltered(self, {actual_rank})
        sender.send_message(f"§bUpdated suffix for rank §e{rank_name} §bto §e{new_suffix}")

        return True

    return True

def updatePermissionsFiltered(self: "PrimeBDS", affected_ranks: set):
    """Reload permissions only for players whose rank is in affected_ranks"""
    for player in self.server.online_players:
        user_data = self.db.get_online_user(player.xuid)
        if user_data and user_data.internal_rank in affected_ranks:
            self.reload_custom_perms(player)