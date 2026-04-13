from typing import TYPE_CHECKING
from time import time

from endstone import ColorFormat
from endstone.event import PlayerChatEvent
import endstone_primebds.utils.internal_permissions_util as perms_util
from endstone_primebds.utils.config_util import load_config
from endstone_primebds.utils.mod_util import format_time_remaining
from endstone_primebds.utils.logging_util import discordRelay

if TYPE_CHECKING:
    from endstone_primebds.primebds import PrimeBDS

def handle_chat_event(self: "PrimeBDS", ev: PlayerChatEvent):
    user_muted = self.db.check_and_update_mute(ev.player.xuid, ev.player.name)
    ip_muted, ip_mute_time, ip_mute_reason = self.db.check_ip_mute(str(ev.player.address))
    if self.globalmute == 1 and not ev.player.has_permission("primebds.globalmute.exempt"):
        ev.player.send_message(f"{ColorFormat.RED}Global chat is currently muted by an admin")
        ev.is_cancelled = True
        return False
    elif ev.player.xuid in self.silentmutes:
        ev.is_cancelled = True
        ev.player.send_message(f"{ColorFormat.RED}Your chats are currently disabled")
        return False

    if user_muted or ip_muted:
        if user_muted:
            user_mod = self.db.get_mod_log(ev.player.xuid)
            ev.player.send_message(
                f"{ColorFormat.GOLD}You are currently muted.\n"
                f"{ColorFormat.GOLD}Expires: {ColorFormat.YELLOW}{format_time_remaining(user_mod.mute_time)}\n"
                f"{ColorFormat.GOLD}Reason: {ColorFormat.YELLOW}{user_mod.mute_reason}")
        else:
            ev.player.send_message(
                f"{ColorFormat.GOLD}You are currently muted.\n"
                f"{ColorFormat.GOLD}Expires: {ColorFormat.YELLOW}{format_time_remaining(ip_mute_time)}\n"
                f"{ColorFormat.GOLD}Reason: {ColorFormat.YELLOW}{ip_mute_reason}")
        ev.is_cancelled = True
        return False
    
    config = load_config()
    user = self.db.get_online_user(ev.player.xuid)

    if user.enabled_sc:
        safe_message = ev.message.replace("{", "{{").replace("}", "}}")
        message = f"{config['modules']['server_messages']['staff_chat_prefix']}{ColorFormat.YELLOW}{ev.player.name}{ColorFormat.GRAY}: {ColorFormat.GOLD}{safe_message}"
        self.server.broadcast(message, "primebds.command.staffchat")
        ev.is_cancelled = True
        return False
    
    enhanced_chat = config["modules"]["server_messages"]["enhanced_chat"]
    chat_cooldown = config["modules"]["server_messages"]["chat_cooldown"]

    current_time = time()
    last_chat_time = self.chat_cooldown.get(ev.player.id, 0)
    time_since_last = current_time - last_chat_time
    time_remaining = chat_cooldown - time_since_last

    if time_since_last >= chat_cooldown:
        self.chat_cooldown[ev.player.id] = current_time
    else:
        ev.player.send_message(f"{ColorFormat.RED}You must wait {time_remaining:.2f}s before chatting again!")
        ev.is_cancelled = True
        return False

    if enhanced_chat:
        def escape_braces(s: str) -> str:
            return s.replace("{", "{{").replace("}", "}}")

        prefix = escape_braces(perms_util.get_prefix(user.internal_rank, perms_util.PERMISSIONS))
        suffix = escape_braces(perms_util.get_suffix(user.internal_rank, perms_util.PERMISSIONS))
        name_tag = escape_braces(ev.player.name_tag)
        safe_msg = escape_braces(ev.message)
        chat_prefix = escape_braces(config['modules']['server_messages']['chat_prefix'])
        ev.format = f"{prefix}{name_tag}{suffix}{chat_prefix}{ColorFormat.RESET}{safe_msg}"

    discordRelay(f"**{ev.player.name}**: {ev.message}", "chat")
    return True

