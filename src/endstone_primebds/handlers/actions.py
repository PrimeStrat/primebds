from typing import TYPE_CHECKING

from endstone.event import PlayerGameModeChangeEvent, PlayerInteractActorEvent, PlayerTeleportEvent, PlayerDeathEvent
from endstone_primebds.utils.config_util import load_config

if TYPE_CHECKING:
    from endstone_primebds.primebds import PrimeBDS

def handle_gamemode_event(self: "PrimeBDS", ev: PlayerGameModeChangeEvent):
    self.db.update_user_data(ev.player.name, "gamemode", ev.new_game_mode.value)

def handle_teleport_event(self: "PrimeBDS", ev: PlayerTeleportEvent):
    config = load_config()
    teleports = config["modules"]["back"]["save_unnatural_teleports"]
    if teleports:
        self.serverdb.set_last_warp(ev.from_location, ev.player.xuid, ev.player.name)
    return

def handle_death_event(self: "PrimeBDS", ev: PlayerDeathEvent):
    config = load_config()
    deaths = config["modules"]["back"]["save_death_locations"]
    if deaths:
        self.serverdb.set_last_warp(ev.player.location, ev.player.xuid, ev.player.name)
    return

def handle_interact_event(self: "PrimeBDS", ev: PlayerInteractActorEvent):
    if not self.gamerules.get("can_interact", 1):
        ev.is_cancelled = True
