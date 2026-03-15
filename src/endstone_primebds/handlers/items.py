from endstone.event import PlayerPickupItemEvent

from typing import TYPE_CHECKING
if TYPE_CHECKING:
    from endstone_primebds.primebds import PrimeBDS

def handle_item_pickup_event(self: "PrimeBDS", ev: PlayerPickupItemEvent):
    if not self.gamerules.get("can_pickup_items", 1):
        ev.is_cancelled = True
    return
