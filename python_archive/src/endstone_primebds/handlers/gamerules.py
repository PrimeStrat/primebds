from typing import TYPE_CHECKING
from endstone_primebds.utils.config_util import load_config

from endstone.event import (
    PlayerEmoteEvent,
    LeavesDecayEvent,
    PlayerSkinChangeEvent,
    PlayerBedEnterEvent
)

if TYPE_CHECKING:
    from endstone_primebds.primebds import PrimeBDS

def handle_emote_event(self: "PrimeBDS", ev: PlayerEmoteEvent):
    if not self.gamerules.get("can_emote", 1):
        ev.is_cancelled = True
    return

def handle_leaves_decay_event(self: "PrimeBDS", ev: LeavesDecayEvent):
    if not self.gamerules.get("can_decay_leaves", 1):
        ev.is_cancelled = True
    return

def handle_skin_change_event(self: "PrimeBDS", ev: PlayerSkinChangeEvent):
    config = load_config()
    if not config["modules"]["server_messages"]["skin_change_messages"]:
        ev.skin_change_message = ""
    if not self.gamerules.get("can_change_skin", 1):
        ev.is_cancelled = True
    return

def handle_bed_enter_event(self: "PrimeBDS", ev: PlayerBedEnterEvent):
    if not self.gamerules.get("can_sleep", 1):
        ev.is_cancelled = True
    return
