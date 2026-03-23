import shlex
from endstone.event import PlayerCommandEvent, ServerCommandEvent
from typing import TYPE_CHECKING

from endstone_primebds.utils.config_util import load_config
from endstone_primebds.utils.logging_util import log, discordRelay
from endstone_primebds.utils.target_selector_util import get_matching_actors

import endstone_primebds.utils.internal_permissions_util as perms_util

if TYPE_CHECKING:
    from endstone_primebds.primebds import PrimeBDS

# Move outside the function to avoid rebuilding every time
MODERATION_COMMANDS = {
    "kick", "ban", "pardon", "unban",
    "permban", "tempban", "tempmute",
    "mute", "ipban", "unmute", "warn",
    "banlist", "ban-ip", "unban-ip", "pardon",
    "pardon-ip"
}
MSG_CMDS = {"me", "tell", "w", "whisper", "msg", "say"}
PARSE_COMMANDS = (
    MODERATION_COMMANDS
    | MSG_CMDS
    | {"op", "deop", "allowlist", "whitelist", "transfer"}
    | {"teleport", "tp", "stop"}
)

def handle_command_preprocess(self: "PrimeBDS", event: PlayerCommandEvent):
    command = event.command
    player = event.player

    try:
        args = shlex.split(command)
    except ValueError as e:
        player.send_message(f"§cInvalid command syntax: {e}")
        return True 
    except IndexError:
        player.send_message("§cInvalid command format")
        return True
    
    config = load_config()
    if config["modules"]["discord_webhook"]["command_logs"]["enabled"]:
        discordRelay(f"**{player.name}** ran: {command}", "cmd")
    
    cmd = args[0].lstrip("/").lower() if args else ""
    if cmd not in PARSE_COMMANDS:
        return 
    
    modules = config.get("modules", {})
    perms_manager = modules.get("permissions_manager", {})
    minecraft_enabled = perms_manager.get("minecraft", True)
    endstone_enabled = perms_manager.get("endstone", True)

    if cmd in MSG_CMDS and command.count("@e") >= 5:
        if player.xuid not in self.crasher_patch_applied:
            for perm in [
                "minecraft.command.me", "minecraft.command.tellraw", "minecraft.command.tell",
                "minecraft.command.w", "minecraft.command.msg"
            ]:
                event.player.add_attachment(self, perm, False)

            self.crasher_patch_applied.add(player.xuid)

        event.is_cancelled = True

        if config["modules"]["me_crasher_patch"]["enabled"]:
            if config["modules"]["me_crasher_patch"]["ban"]:
                self.server.dispatch_command(
                    self.server.command_sender,
                    f"tempban {player.name} 7 day Crasher Exploit"
                )
            else:
                log(self, f"§6Player §e{player.name} §6was kicked due to §eCrasher Exploit", "mod")
                player.kick("Disconnected")
        return False

    is_exempt = False
    if cmd in MODERATION_COMMANDS and len(args) >= 2 and "@" not in args[1]:
        target = self.db.get_offline_user(args[1])
        if target:
            if (
                (cmd == "warn" and perms_util.check_perms(self, target, "primebds.exempt.warn")) or
                (cmd == "kick" and perms_util.check_perms(self, target, "primebds.exempt.kick")) or
                (cmd in {"mute", "tempmute"} and perms_util.check_perms(self, target, "primebds.exempt.mute")) or
                (cmd in {"permban", "tempban", "ipban", "ban", "ban-ip"} and perms_util.check_perms(self, target, "primebds.exempt.ban"))
            ):
                player.send_message(f"§6Player §e{target.name} §6is exempt from §e{cmd}")
                is_exempt = True
    if is_exempt:
        event.is_cancelled = True
        return True

    if (cmd == "teleport" or cmd == "tp") and minecraft_enabled and player.has_permission("minecraft.command.teleport") and not player.is_op: # Bypass TP exception
        self.server.dispatch_command(self.server.command_sender, f"execute as \"{player.name}\" at \"{player.name}\" run {command}")
        event.is_cancelled = True
        return False
    elif cmd == "kick":
        selector = args[1].strip('"')
        reason = " ".join(args[2:]) 
        matched = get_matching_actors(self, selector, player)

        if not matched:
            return True

        for pl in matched:
            target = self.db.get_online_user(pl.xuid)

            if perms_util.check_perms(self, target, "primebds.exempt.kick"):
                player.send_message(f"§6Player §e{target.name} §6is exempt from §e{cmd}")
                continue

            try:
                pl.kick(reason)
                player.send_message(f"§6Player §e{target.name} §6was kicked for §e\"{reason}\"")
            except Exception as e:
                player.send_error_message(f"Failed to kick {target.name}: {e}")

        event.is_cancelled = True
        return False
    elif cmd == "stop":
        for player in self.server.online_players:
            player.kick(config["modules"]["join_leave_messages"]["shutdown"])
        return False
    elif cmd == "banlist" and endstone_enabled:
        if len(args) < 2:
            player.perform_command(f'flist banned')
            player.perform_command(f'flist ipbanned')
        else:
            if args[1] == "players":
                player.perform_command(f'flist banned')
            else:
                player.perform_command(f'flist ipbanned')
        event.is_cancelled = True
        return False
    elif cmd == "ban" and len(args) > 1 and endstone_enabled:
        player.perform_command(f'permban \"{args[1]}\"')
        event.is_cancelled = True
        return False
    elif cmd == "ban-ip" and len(args) > 1 and endstone_enabled:
        if len(args) > 2:
            player.perform_command(f'ipban \"{args[1]}\" ip \"{args[2]}\"')
        else:
            player.perform_command(f'ipban \"{args[1]}\" ip')
        event.is_cancelled = True
        return False
    elif cmd in {"unban", "pardon", "unban-ip", "pardon-ip"} and len(args) > 1 and endstone_enabled:
        if cmd in {"unban-ip", "pardon-ip"}:
            player.perform_command(f'removeban ip \"{args[1]}\"')
            event.is_cancelled = True
            return False
        player.perform_command(f'removeban \"{args[1]}\"')
        event.is_cancelled = True
        return False
    elif cmd == "op" and len(args) > 1:
        self.server.dispatch_command(self.server.command_sender, f"rank set \"{args[1]}\" operator")
        event.is_cancelled = True
        return False
    elif cmd == "deop" and len(args) > 1:
        self.server.dispatch_command(self.server.command_sender, f"rank set \"{args[1]}\" default")
        event.is_cancelled = True
        return False
    elif cmd in {"allowlist", "whitelist"} and len(args) > 1:
        sub = args[1]
        if sub in {"add", "remove"} and len(args) > 2:
            player.perform_command(f'alist {sub} \"{args[2]}\"')
        elif sub == "list":
            player.perform_command("alist list")
        elif sub in {"on", "off"}:
            player.send_message("Mojang has this feature disabled")
        event.is_cancelled = True
        return False
    elif cmd == "transfer" and len(args) > 2:
        port = args[3] if len(args) >= 4 else 19132
        player.perform_command(f"send \"{args[1]}\" {args[2]} {port}")
        event.is_cancelled = True
        return False

    if cmd in MSG_CMDS and len(args) > 1:
        mod_log = self.db.get_mod_log(player.xuid)
        if mod_log:
            if mod_log.is_muted == 1:
                self.db.check_and_update_mute(player.xuid, player.name)
                event.is_cancelled = True
                return True
        target = args[1]
        if "@" in target:
            player.send_message("§cTarget selectors are invalid for this command")
            event.is_cancelled = True
            return True
        
        if cmd == "me":
            return True
        
        self.db.update_user_data(player.name, 'last_messaged', target)
        target_user = self.db.get_offline_user(target)
        if target_user is not None:
            if target_user.enabled_mt == 0 and not player.has_permission("primebds.exempt.msgtoggle"):
                player.send_message("§cThis player has private messages disabled")
                event.is_cancelled = True
                return True

        message = " ".join(args[2:]) if len(args) > 2 else ""
        discordRelay(f"**{player.name} -> {target}**: {message}", "chat")

        if config["modules"]["server_messages"]["enhanced_whispers"]:
            online_target = self.server.get_player(target)
            player.send_message(f"{config['modules']['server_messages']['whisper_prefix']}§7To {online_target.name}: §o{message}")
            online_target.send_message(f"{config['modules']['server_messages']['whisper_prefix']}§7From {player.name}: §o{message}")
            event.is_cancelled = True

        for pl in self.server.online_players:  
            user = self.db.get_online_user(pl.xuid)
            if user:
                if user.enabled_ss == 1 and pl.has_permission("primebds.command.socialspy"):
                    pl.send_message(f"{config['modules']['server_messages']['social_spy_prefix']}§8[§r{player.name} §7-> §r{target}§8] §7{message}")

def handle_server_command_preprocess(self: "PrimeBDS", event: ServerCommandEvent):
    args = event.command.split()
    if not args:
        return

    cmd = args[0].lstrip("/").lower()
    if cmd not in PARSE_COMMANDS:
        return 

    # Simple remap commands
    remap_commands = {
        "ban": lambda: ["permban"] + args[1:],
        "unban": lambda: ["removeban"] + args[1:],
        "pardon": lambda: ["removeban"] + args[1:],
        "op": lambda: ["rank", "set", f"\"{args[1]}\"", "operator"],
        "deop": lambda: ["rank", "set", f"\"{args[1]}\"", "default"],
    }

    # Allowlist/Whitelist handling
    allowlist_aliases = {"allowlist", "whitelist"}

    if cmd == "stop":
        for player in self.server.online_players:
            player.kick("Server was shutdown!")
        return False

    if cmd in remap_commands:
        new_args = remap_commands[cmd]()
        self.server.dispatch_command(self.server.command_sender, " ".join(new_args))
        event.is_cancelled = True
        return False

    if cmd in allowlist_aliases and len(args) > 1:
        if args[1] in {"add", "remove"} and len(args) > 2:
            self.server.dispatch_command(self.server.command_sender, f'alist {args[1]} \"{args[2]}\"')
        elif args[1] in {"on", "off", "list"}:
            self.server.dispatch_command(self.server.command_sender, f'alist {args[1]}')
        event.is_cancelled = True
        return False

    # Transfer command
    if cmd == "transfer" and len(args) >= 3:
        port = args[3] if len(args) >= 4 else 19132
        if "@s" not in args[0]:
            self.server.dispatch_command(self.server.command_sender, f"send \"{args[1]}\" {args[2]} {port}")
        event.is_cancelled = True
        return False
