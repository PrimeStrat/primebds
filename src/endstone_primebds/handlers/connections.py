import json
import os
import time

from endstone.util import Vector
from endstone.event import PlayerLoginEvent, PlayerJoinEvent, PlayerQuitEvent, PlayerKickEvent
from typing import TYPE_CHECKING
from datetime import datetime

from endstone_primebds.utils.config_util import load_config
from endstone_primebds.utils.mod_util import format_time_remaining, ban_message
from endstone_primebds.utils.logging_util import log, discordRelay
from endstone.inventory import ItemStack

import endstone_primebds.utils.internal_permissions_util as perms_util

if TYPE_CHECKING:
    from endstone_primebds.primebds import PrimeBDS

def handle_login_event(self: "PrimeBDS", ev: PlayerLoginEvent):

    self.crasher_patch_applied.discard(ev.player.xuid)

    # Ban System: ENHANCEMENT
    now = datetime.now()

    player_xuid = ev.player.xuid
    player_ip = str(ev.player.address)

    mod_log = self.db.get_mod_log(player_xuid)
    is_ip_banned = self.db.check_ip_ban(player_ip)

    # Handle Name Ban
    if self.serverdb.check_nameban(ev.player.name):
        name_ban_log = self.serverdb.get_ban_info(ev.player.name)
        banned_time = datetime.fromtimestamp(name_ban_log.banned_time)
        if now >= banned_time:
            self.serverdb.remove_name(player_ip)
        else:  
            formatted_expiration = format_time_remaining(name_ban_log.banned_time)
            message = ban_message(self.server.level.name, formatted_expiration, name_ban_log.ban_reason)
            ev.kick_message = message
            ev.is_cancelled = True 
    
    # Handle IP Ban
    if is_ip_banned:
        banned_time = datetime.fromtimestamp(mod_log.banned_time)
        if now >= banned_time:  # IP Ban has expired
            self.db.remove_ban(player_ip)
        else:  # IP Ban is still active
            formatted_expiration = format_time_remaining(mod_log.banned_time)
            message = ban_message(self.server.level.name, formatted_expiration, "IP Ban - " + mod_log.ban_reason)
            ev.kick_message = message
            ev.is_cancelled = True 

    # Handle XUID Ban
    elif mod_log:
        if mod_log.is_banned:  # Only proceed if the player is banned
            banned_time = datetime.fromtimestamp(mod_log.banned_time)
            if now >= banned_time:  # Ban has expired
                self.db.remove_ban(player_xuid)
            else:  # Ban is still active
                formatted_expiration = format_time_remaining(mod_log.banned_time)
                message = ban_message(self.server.level.name, formatted_expiration, mod_log.ban_reason)
                ev.kick_message = message
                ev.is_cancelled = True 

    return

def handle_join_event(self: "PrimeBDS", ev: PlayerJoinEvent):

    config = load_config()
    send_on_connect = config["modules"]["join_leave_messages"]["send_on_connection"]
    join_message = config["modules"]["join_leave_messages"]["join_message"]
    rank_meta_nametags = config["modules"]["server_messages"]["rank_meta_nametags"] 
    motd_on_connect = config["modules"]["message_of_the_day"]["send_message_of_the_day_on_connect"] 
    motd = config["modules"]["message_of_the_day"]["message_of_the_day_command"]

    if send_on_connect:
        ev.join_message = f"{join_message.replace('{player}', ev.player.name)}"

    if motd_on_connect:
        ev.player.send_message(motd)

    # Update Saved Data
    self.db.save_user(ev.player)
    self.db.update_user_data(ev.player.name, "is_afk", 0)
    self.db.update_user_data(ev.player.name, 'last_join', int(time.time()))
    self.db.check_alts(ev.player.xuid, ev.player.name, str(ev.player.address), ev.player.device_id)
    self.server.scheduler.run_task(self, self.reload_custom_perms(ev.player), 1)

    user = self.db.get_online_user(ev.player.xuid)

    # Ban System: ENHANCEMENT
    mod_log = self.db.get_mod_log(ev.player.xuid)
    if mod_log:
        if mod_log.is_banned:
            ev.join_message = "" 
        else:
            # Handle Alt Detection
            alts = self.db.get_alts(str(ev.player.address), ev.player.device_id, ev.player.xuid)
            if len(alts) > 0:
                alt_names = ", ".join(alt["name"] for alt in alts)
                message = f"§6Alt Detected: §e{ev.player.name} §7-> §8[§7{alt_names}§8]"
                log(self, message, "mod", toggles=["enabled_as"])

            # Handle Activity
            self.sldb.start_session(ev.player.xuid, ev.player.name, int(time.time()))

    warning = self.db.get_latest_active_warning(ev.player.xuid, ev.player.name)
    if warning:
        reason = warning.get("warn_reason", "Negative Behavior")
        ev.player.send_message(f"§6Reminder: You were recently warned for §e{reason}")

    if rank_meta_nametags:
        prefix = perms_util.get_prefix(user.internal_rank, perms_util.PERMISSIONS)
        suffix = perms_util.get_suffix(user.internal_rank, perms_util.PERMISSIONS)
        ev.player.name_tag = prefix+ev.player.name+suffix

    discordRelay(f"**{ev.player.name}** has joined the server ***({len(self.server.online_players)}/{self.server.max_players})***", "connections")
    return

def handle_leave_event(self: "PrimeBDS", ev: PlayerQuitEvent):

    config = load_config()
    send_on_connect = config["modules"]["join_leave_messages"]["send_on_connection"]
    leave_message = config["modules"]["join_leave_messages"]["leave_message"] 

    if send_on_connect:
        ev.quit_message = f"{leave_message.replace('{player}', ev.player.name)}"

    # Update Data On Leave
    self.db.update_user_data(ev.player.name, 'xp', ev.player.total_exp)
    self.db.update_user_data(ev.player.name, 'last_leave', int(time.time()))
    self.db.update_user_data(ev.player.name, "is_afk", 0)
    self.db.save_inventory(ev.player)
    self.db.save_enderchest(ev.player)

    if ev.player.unique_id in self.vanish_state:
        del self.vanish_state[ev.player.unique_id]

    # Ban System: ENHANCEMENT
    mod_log = self.db.get_mod_log(ev.player.xuid)
    if mod_log:
        if mod_log.is_banned:
            ev.quit_message = ""  # Remove join message
        else:
            # User Log
            self.sldb.end_session(ev.player.xuid, int(time.time()))
            rounded_x = round(ev.player.location.x)
            rounded_y = round(ev.player.location.y)
            rounded_z = round(ev.player.location.z)
            rounded_coords = Vector(rounded_x, rounded_y, rounded_z)
            self.db.update_user_data(ev.player.name, 'last_logout_pos', rounded_coords)
            self.db.update_user_data(ev.player.name, 'last_logout_dim', ev.player.dimension.name)

    discordRelay(f"**{ev.player.name}** has left the server ***({len(self.server.online_players)-1}/{self.server.max_players})***", "connections")
    return

def handle_kick_event(self: "PrimeBDS", ev: PlayerKickEvent):
    self.sldb.end_session(ev.player.xuid, int(time.time()))