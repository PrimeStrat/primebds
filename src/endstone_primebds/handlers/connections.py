import json
import os
import time

from endstone.util import Vector
from endstone.event import PlayerLoginEvent, PlayerJoinEvent, PlayerQuitEvent, PlayerKickEvent
from typing import TYPE_CHECKING
from datetime import datetime
from endstone_primebds.handlers.intervals import start_jail_check_if_needed, stop_jail_check_if_not_needed
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
    start_jail_check_if_needed(self)

    user = self.db.get_online_user(ev.player.xuid)
    if user:
        self.vanish_state[ev.player.unique_id] = bool(user.is_vanish)
    else:
        self.vanish_state[ev.player.unique_id] = False

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

    # Hide Vanish
    if self.db.get_online_user(ev.player.xuid).is_vanish:
        ev.join_message = ""

    warning = self.db.get_latest_active_warning(ev.player.xuid, ev.player.name)
    if warning:
        reason = warning.get("warn_reason", "Negative Behavior")
        ev.player.send_message(f"§6Reminder: You were recently warned for §e{reason}")

    if rank_meta_nametags:
        prefix = perms_util.get_prefix(user.internal_rank, perms_util.PERMISSIONS)
        suffix = perms_util.get_suffix(user.internal_rank, perms_util.PERMISSIONS)
        ev.player.name_tag = prefix+ev.player.name+suffix

    discordRelay(f"**{ev.player.name}** has joined the server ***({len(self.server.online_players)}/{self.server.max_players})***", "connections")
    check_unset_scoreboards(self)
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
    stop_jail_check_if_not_needed(self)

    if ev.player.unique_id in self.vanish_state:
        del self.vanish_state[ev.player.unique_id]

    # Ban System: ENHANCEMENT
    mod_log = self.db.get_mod_log(ev.player.xuid)
    if mod_log:
        if mod_log.is_jailed:
            air = ItemStack("minecraft:air", 1)
            ev.player.inventory.helmet = air
            ev.player.inventory.chestplate = air
            ev.player.inventory.leggings = air
            ev.player.inventory.boots = air
            ev.player.inventory.item_in_off_hand = air
            jail = self.serverdb.get_jail(mod_log.jail, self.server)
            ev.player.inventory.clear()
            ev.player.teleport(jail["pos"])
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

    online_user = self.db.get_online_user(ev.player.xuid)
    if online_user and getattr(online_user, "is_vanish", False):
        ev.quit_message = ""

    discordRelay(f"**{ev.player.name}** has left the server ***({len(self.server.online_players)-1}/{self.server.max_players})***", "connections")
    return

def handle_kick_event(self: "PrimeBDS", ev: PlayerKickEvent):
    self.sldb.end_session(ev.player.xuid, int(time.time()))

def check_unset_scoreboards(self):
    current_dir = os.path.dirname(os.path.abspath(__file__))
    while not (
        os.path.exists(os.path.join(current_dir, 'plugins')) and
        os.path.exists(os.path.join(current_dir, 'worlds'))
    ):
        current_dir = os.path.dirname(current_dir)

    scoreboard_data_folder = os.path.join(current_dir, 'plugins/primebds_data', 'scoreboard_data')
    if not os.path.exists(scoreboard_data_folder):
        return  # Folder doesn't exist, nothing to check

    # Check if all scoreboard files are fully loaded
    all_fully_loaded = True
    for filename in os.listdir(scoreboard_data_folder):
        if filename.endswith(".json"):
            full_path = os.path.join(scoreboard_data_folder, filename)
            with open(full_path, 'r') as f:
                data = json.load(f)
            # If any objective in this file is not fully loaded, we continue processing
            for obj_data in data.values():
                if not obj_data.get("is_fully_loaded", False):
                    all_fully_loaded = False
                    break
            if not all_fully_loaded:
                break

    if all_fully_loaded:
        return

    # Map online players by xuid for quick lookup
    online_players_by_xuid = {player.xuid: player for player in self.server.online_players}

    for filename in os.listdir(scoreboard_data_folder):
        if not filename.endswith('.json'):
            continue

        full_path = os.path.join(scoreboard_data_folder, filename)
        try:
            with open(full_path, 'r') as f:
                data = json.load(f)
        except Exception as e:
            print(f"Could not read scoreboard file {filename}: {e}")
            continue

        file_modified = False

        for obj_name, obj_data in data.items():
            if not isinstance(obj_data, dict):
                continue

            entries = obj_data.get("entries", {})
            if not isinstance(entries, dict):
                continue

            all_loaded = True
            objective = self.server.scoreboard.get_objective(obj_name)
            if not objective:
                # If objective not present, add it with DUMMY criteria
                from endstone.scoreboard import Criteria
                criteria = Criteria.DUMMY
                display_name = obj_data.get("display_name", obj_name)
                objective = self.server.scoreboard.add_objective(obj_name, criteria, display_name)

            # Iterate all entries
            for entry_key, entry_data in entries.items():
                loaded = entry_data.get("loaded", False)
                if loaded:
                    continue

                if entry_key.isdigit() and len(entry_key) >= 12:
                    player = online_players_by_xuid.get(entry_key)
                    if player:
                        score_obj = objective.get_score(player)
                        score_obj.value = entry_data.get("value", 0)
                        entry_data["loaded"] = True
                        file_modified = True
                    else:
                        all_loaded = False
                else:
                    # Non XUID entries should already be loaded immediately
                    # Mark loaded True to avoid repeated checks
                    entry_data["loaded"] = True
                    file_modified = True

            # Update is_fully_loaded based on whether all entries are loaded
            if all_loaded and obj_data.get("is_fully_loaded") != True:
                obj_data["is_fully_loaded"] = True
                file_modified = True
            elif not all_loaded and obj_data.get("is_fully_loaded") != False:
                obj_data["is_fully_loaded"] = False
                file_modified = True

        if file_modified:
            try:
                with open(full_path, 'w') as f:
                    json.dump(data, f, indent=4)
                print(f"[PrimeBDS] Updated scoreboard file '{filename}' after loading missing entries.")
            except Exception as e:
                print(f"[PrimeBDS] Could not save updated scoreboard file {filename}: {e}")
