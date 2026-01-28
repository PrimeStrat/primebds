from endstone import GameMode
from endstone.level import Location
from endstone_primebds.utils.config_util import load_config

from endstone_primebds.utils.intervals_util import IntervalManager

from typing import TYPE_CHECKING
if TYPE_CHECKING:
    from endstone_primebds.primebds import PrimeBDS

import threading

def init_intervals(self: "PrimeBDS"):
    """Actual interval setup, safe to run in its own thread."""
    setup_intervals(self)
    start_jail_check_if_needed(self)
    start_afk_check_if_needed(self)

def setup_intervals(self: "PrimeBDS"):
    """Prepare system, but don't start it until needed."""
    self.interval_manager = IntervalManager(self, tick_interval=20)
    self.interval_manager.add_check(check_jailed)
    self.interval_manager.add_check(check_afk)

def init_afk_intervals(self: "PrimeBDS"):
    """Initialize AFK interval system in a separate thread."""
    def worker():
        setup_afk_intervals(self)
        start_afk_check_if_needed(self)
    t = threading.Thread(target=worker, name="AfkIntervalInit")
    t.start()

def setup_afk_intervals(self: "PrimeBDS"):
    """Prepare AFK check system but don't start it until needed."""
    self.afk_interval_manager = IntervalManager(self, tick_interval=20)
    self.afk_interval_manager.add_check(check_afk)

def check_afk(self: "PrimeBDS"):
    """Check and handle player AFK states and auto-detection."""
    config = load_config()
    auto_detect = config["modules"]["afk"]["constantly_check_afk_status"]
    idle_threshold = config["modules"]["afk"]["idle_threshold"]
    broadcast = config["modules"]["afk"]["broadcast_afk_status"]

    for player in self.server.online_players:
        try:
            row = self.db.execute("SELECT is_afk FROM users WHERE xuid = ?", (player.xuid,)).fetchone()
            is_afk = bool(row[0]) if row else False

            if player.xuid not in self.afk_cache:
                self.afk_cache[player.xuid] = {"pos": player.location, "idle_time": 0}

            last_loc = self.afk_cache[player.xuid]["pos"]
            current_loc = player.location

            dist = current_loc.distance(last_loc)

            if dist >= 0.5:
                self.afk_cache[player.xuid]["idle_time"] = 0

                if is_afk:
                    self.db.update_user_data(player.name, "is_afk", 0)
                    msg = f"§e{player.name} is no longer AFK"

                    if broadcast:
                        for p in self.server.online_players:
                            p.send_message(msg)
                    else:
                        player.send_message(msg)

                self.afk_cache[player.xuid]["pos"] = current_loc
                continue

            if auto_detect:
                self.afk_cache[player.xuid]["idle_time"] += 1
                idle_time = self.afk_cache[player.xuid]["idle_time"]

                if idle_time >= idle_threshold and not is_afk:
                    self.db.update_user_data(player.name, "is_afk", 1)
                    msg = f"§e{player.name} is now AFK"

                    if broadcast:
                        self.server.broadcast_message(msg)
                    else:
                        player.send_message(msg)
            else:
                stop_afk_check_if_not_needed(self)

            self.afk_cache[player.xuid]["pos"] = current_loc

        except Exception as e:
            print(f"[PrimeBDS] Error in AFK check for {getattr(player, 'name', 'Unknown')}: {e}")

def start_afk_check_if_needed(self: "PrimeBDS"):
    """Start AFK interval if needed (AFK players or config says to constantly check)."""
    def main_thread_check():
        config = load_config()
        auto_detect = config["modules"]["afk"]["constantly_check_afk_status"]

        row = self.db.execute("SELECT 1 FROM users WHERE is_afk = 1 LIMIT 1").fetchone()
        any_afk = row is not None

        if any_afk or auto_detect:
            if not getattr(self.afk_interval_manager, "_task_id", None):
                self.afk_interval_manager.start()

    self.server.scheduler.run_task(self, main_thread_check, 0)

def stop_afk_check_if_not_needed(self: "PrimeBDS"):
    """Stop AFK interval if no one is AFK and config doesn't require constant checking."""
    config = load_config()
    auto_detect = config["modules"]["afk"]["constantly_check_afk_status"]

    row = self.db.execute("SELECT 1 FROM users WHERE is_afk = 1 LIMIT 1").fetchone()
    any_afk = row is not None

    if not any_afk and not auto_detect:
        if getattr(self.afk_interval_manager, "_task_id", None):
            self.afk_interval_manager.stop()

def init_jail_intervals(self: "PrimeBDS"):
    """Initialize the jail interval system in a separate thread."""
    def worker():
        init_intervals(self)

    t = threading.Thread(target=worker, name="JailIntervalInit")
    t.start()

def refresh_jail_cache(self: "PrimeBDS", player):
    """Update jail cache from DB only when needed."""
    is_jailed, is_expired = self.db.check_jailed(player.xuid)
    if is_jailed:
        mod_user = self.db.get_mod_log(player.xuid)
        self.jail_cache[player.xuid] = {
            "is_jailed": True,
            "is_expired": is_expired,
            "data": mod_user,
        }
    else:
        self.jail_cache[player.xuid] = {"is_jailed": False, "is_expired": False, "data": None}

def start_jail_check_if_needed(self: "PrimeBDS"):
    """Run this on the main tick scheduler to avoid threading issues."""
    def main_thread_check():
        if any(self.db.check_jailed(p.xuid)[0] for p in self.server.online_players):
            if not getattr(self.interval_manager, "_task_id", None):
                self.interval_manager.start()
    self.server.scheduler.run_task(self, main_thread_check, 0)

def stop_jail_check_if_not_needed(self: "PrimeBDS"):
    """Stop interval if no online player is jailed."""
    if not any(self.db.check_jailed(p.xuid)[0] for p in self.server.online_players):
        if getattr(self.interval_manager, "_task_id", None):
            self.interval_manager.stop()

def check_jailed(self: "PrimeBDS"):
    """Handle players whose jail time has expired."""
    for player in self.server.online_players:
        try:
            cached = self.jail_cache.get(player.xuid)

            if not cached or (cached["is_jailed"] and cached["is_expired"]):
                refresh_jail_cache(self, player)
                cached = self.jail_cache[player.xuid]

            if cached["is_jailed"] and player.id is not self.isgod:
                self.isgod.add(player.id)

            if cached["is_jailed"] and cached["is_expired"]:
                mod_user = cached["data"]

                x, y, z = map(float, mod_user.return_jail_pos.split(","))
                loc = Location(
                    self.server.level.get_dimension(mod_user.return_jail_dim),
                    x, y, z,
                    player.location.pitch,
                    player.location.yaw,
                )
                player.teleport(loc)
                player.game_mode = GameMode(int(mod_user.jail_gamemode))
                self.db.remove_jail(player.name)
                self.server.dispatch_command(
                    self.server.command_sender,
                    f'effect "{player.name}" clear saturation'
                )
                player.send_message("§6You were freed from jail, time expired!")

                self.db.load_inventory(player)
                self.jail_cache[player.xuid] = {"is_jailed": False, "is_expired": False, "data": None}

        except Exception as e:
            print(f"[PrimeBDS] Error handling player {getattr(player, 'name', 'Unknown')}: {e}")

        stop_jail_check_if_not_needed(self)

def stop_intervals(self: "PrimeBDS"):
    """Stop all periodic checks safely (on shutdown)."""
    if hasattr(self, "interval_manager"):
        self.interval_manager.stop()

def recheck_all_intervals(self: "PrimeBDS"):
    """
    Re-evaluate whether AFK and Jail intervals should be running.
    Call this after config reloads or any mid-game setting changes.
    """
    config = load_config()

    any_jailed = any(self.db.check_jailed(p.xuid)[0] for p in self.server.online_players)

    if any_jailed:
        if not getattr(self.interval_manager, "_task_id", None):
            self.interval_manager.start()
    else:
        if getattr(self.interval_manager, "_task_id", None):
            self.interval_manager.stop()

    auto_detect = config["modules"]["afk"]["constantly_check_afk_status"]
    any_afk = self.db.execute("SELECT 1 FROM users WHERE is_afk = 1 LIMIT 1").fetchone() is not None

    if any_afk or auto_detect:
        if not getattr(self.afk_interval_manager, "_task_id", None):
            self.afk_interval_manager.start()
    else:
        if getattr(self.afk_interval_manager, "_task_id", None):
            self.afk_interval_manager.stop()
