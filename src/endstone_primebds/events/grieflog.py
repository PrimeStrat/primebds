import time

from endstone.event import BlockPlaceEvent, BlockBreakEvent, PlayerInteractEvent
from typing import TYPE_CHECKING

from endstone_primebds.utils.configUtil import load_config
from endstone_primebds.utils.loggingUtil import sendgrieflog
from endstone_primebds.utils.dbUtil import grieflog

if TYPE_CHECKING:
    from endstone_primebds.primebds import PrimeBDS

def handle_block_break(self: "PrimeBDS", ev: BlockBreakEvent):
    config = load_config()
    is_gl_enabled = config["modules"]["grieflog"]["enabled"]

    if is_gl_enabled:
        dbgl = grieflog("grieflog.db")

        if dbgl.get_user_toggle(ev.player.xuid, ev.player.name)[3]:
            logs = dbgl.get_logs_by_coordinates(ev.block.x, ev.block.y, ev.block.z)
            sendgrieflog(logs, ev.player)
            ev.is_cancelled = True
        else:
            block_states = list(ev.block.data.block_states.values())
            formatted_block_states = ", ".join(map(str, block_states))
            dbgl.log_action(ev.player.xuid, ev.player.name, "Block Break", ev.block.location, int(time.time()), ev.block.data.type, formatted_block_states)

        dbgl.close_connection()

    return True

def handle_block_place(self: "PrimeBDS", ev: BlockPlaceEvent):
    config = load_config()
    is_gl_enabled = config["modules"]["grieflog"]["enabled"]

    if is_gl_enabled:
        dbgl = grieflog("grieflog.db")

        if dbgl.get_user_toggle(ev.player.xuid, ev.player.name)[3]:
            logs = dbgl.get_logs_by_coordinates(ev.block.x, ev.block.y, ev.block.z)
            sendgrieflog(logs, ev.player)
            ev.is_cancelled = True
        else:
            placed_block = ev.block_placed_state
            block_states = list(ev.block.data.block_states.values())
            formatted_block_states = ", ".join(map(str, block_states))
            dbgl.log_action(ev.player.xuid, ev.player.name, "Block Place", placed_block.location, int(time.time()), placed_block.type, formatted_block_states)

        dbgl.close_connection()
    return True

last_interaction_time = {}
def handle_player_interact(self: "PrimeBDS", ev: PlayerInteractEvent):
    config = load_config()
    is_gl_enabled = config["modules"]["grieflog"]["enabled"]

    if is_gl_enabled:
        dbgl = grieflog("grieflog.db")

        current_time = time.time()
        last_time = last_interaction_time.get(ev.player.xuid, 0)

        if current_time - last_time < 0.5:
            return True

        last_interaction_time[ev.player.xuid] = current_time

        if ev.block is not None:
            types_to_check = [
                "chest", "barrel", "furnace", "table", "crafter", "shulker", "smoker",
                "dispenser", "dropper", "hopper", "command", "lectern", "stonecutter",
                "grindstone", "anvil", "beacon"
            ]

            if dbgl.get_user_toggle(ev.player.xuid, ev.player.name)[3]:
                logs = dbgl.get_logs_by_coordinates(ev.block.x, ev.block.y, ev.block.z)
                sendgrieflog(logs, ev.player)
                ev.is_cancelled = True
            elif any(item in ev.block.data.type for item in types_to_check):
                block_states = list(ev.block.data.block_states.values())
                formatted_block_states = ", ".join(map(str, block_states))
                dbgl.log_action(
                    ev.player.xuid,
                    ev.player.name,
                    "Opened Container",
                    ev.block.location,
                    int(time.time()),
                    ev.block.data.type,
                    formatted_block_states
                )

        dbgl.close_connection()
    return True