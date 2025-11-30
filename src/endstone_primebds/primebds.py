from collections import defaultdict
import os
import threading
import time
import traceback
from endstone import Player
from endstone.plugin import Plugin
from endstone.command import Command, CommandSender

from endstone_primebds.commands import (
    preloaded_commands,
    preloaded_permissions,
    preloaded_handlers
)

from endstone_primebds.commands.Server.monitor import clear_all_monitor_intervals
from endstone_primebds.commands.Misc.blockscan import clear_all_blockscan_intervals
from endstone_primebds.utils.config_util import load_config
from endstone_primebds.utils.economy_utils import get_eco_link
from endstone_primebds.utils.db_util import UserDB, sessionDB, ServerDB, User, ModLog, ServerData
import endstone_primebds.utils.internal_permissions_util as perms_util

def plugin_text():
    print(
        """

██████╗░██████╗░██╗███╗░░░███╗███████╗██████╗░██████╗░░██████╗
██╔══██╗██╔══██╗██║████╗░████║██╔════╝██╔══██╗██╔══██╗██╔════╝
██████╔╝██████╔╝██║██╔████╔██║█████╗░░██████╦╝██║░░██║╚█████╗░
██╔═══╝░██╔══██╗██║██║╚██╔╝██║██╔══╝░░██╔══██╗██║░░██║░╚═══██╗
██║░░░░░██║░░██║██║██║░╚═╝░██║███████╗██████╦╝██████╔╝██████╔╝
╚═╝░░░░░╚═╝░░╚═╝╚═╝╚═╝░░░░░╚═╝╚══════╝╚═════╝░╚═════╝░╚═════╝░                  

Prime BDS Loaded!
        """
    )

# EVENT & HANDLER IMPORTS
from endstone.event import (EventPriority, event_handler, PlayerLoginEvent, PlayerJoinEvent, PlayerQuitEvent,
                            ServerCommandEvent, PlayerCommandEvent, PlayerChatEvent, ActorDamageEvent, ActorKnockbackEvent, PacketSendEvent, PlayerPickupItemEvent, 
                            PlayerGameModeChangeEvent, PlayerInteractActorEvent, PlayerDropItemEvent, PlayerItemConsumeEvent, PacketReceiveEvent,
                            ServerLoadEvent, PlayerKickEvent, PlayerBedEnterEvent, PlayerEmoteEvent, 
                            LeavesDecayEvent, PlayerSkinChangeEvent, PlayerTeleportEvent, PlayerDeathEvent)
from endstone_primebds.handlers.chat import handle_chat_event
from endstone_primebds.handlers.preprocesses import handle_command_preprocess, handle_server_command_preprocess
from endstone_primebds.handlers.connections import handle_login_event, handle_join_event, handle_leave_event, handle_kick_event
from endstone_primebds.handlers.combat import handle_kb_event, handle_damage_event
from endstone_primebds.handlers.multiworld import start_additional_servers, stop_additional_servers, is_nested_multiworld_instance
from endstone_primebds.handlers.intervals import stop_intervals, init_jail_intervals, init_afk_intervals
from endstone_primebds.handlers.packets import handle_packetsend_event, handle_packetreceive_event
from endstone_primebds.handlers.actions import handle_gamemode_event, handle_interact_event, handle_teleport_event, handle_death_event
from endstone_primebds.handlers.items import handle_item_pickup_event, handle_item_use, handle_item_drop_event
from endstone_primebds.handlers.gamerules import handle_bed_enter_event, handle_emote_event, handle_leaves_decay_event, handle_skin_change_event

class PrimeBDS(Plugin):
    api_version = "0.10"
    authors = ["PrimeStrat"]
    name = "primebds"
    description = "An essentials plugin for diagnostics, stability, and quality of life on Minecraft Bedrock Edition."
    website = "https://github.com/PrimeStrat/primebds"
    
    commands = preloaded_commands
    permissions = preloaded_permissions
    handlers = preloaded_handlers

    def __init__(self):
        super().__init__()
        # Command Controls
        self.monitor_intervals = {}
        self.blockscan_intervals = {}
        self.packets_sent_count = defaultdict(int)
        self.gamerules = {}
        self.cached_players = set()
        self.vanish_state = {}
        self.jail_cache = {}
        self.afk_cache = {}
        self.packet_last_sample = {
            "time": time.time(),
            "counts": {}
        }
        self.globalmute = 0
        self.chat_cooldown = {}
        self.silentmutes = set()
        self.isgod = set()
        self.crasher_patch_applied = set()

        # Multiworld Handler
        self.multiworld_processes = {}
        self.multiworld_ports = {}
        self.multiworld_lock = threading.Lock()

        # Combat Handler
        self.entity_damage_cooldowns = {}
        self.entity_last_hit = {}
        self.entity_enchant_hit = {}

        # DB
        self.db = UserDB("users.db")
        self.sldb = sessionDB("sessionlog.db")
        self.serverdb = ServerDB("server.db")

    # EVENT HANDLER
    @event_handler
    def on_player_death(self, ev: PlayerDeathEvent):
        handle_death_event(self, ev)

    @event_handler
    def on_player_teleport(self, ev: PlayerTeleportEvent):
        handle_teleport_event(self, ev)

    @event_handler
    def on_player_bed_enter(self, ev: PlayerBedEnterEvent):
        handle_bed_enter_event(self, ev)

    @event_handler
    def on_player_emote(self, ev: PlayerEmoteEvent):
        handle_emote_event(self, ev)

    @event_handler
    def on_player_skin_change(self, ev: PlayerSkinChangeEvent):
        handle_skin_change_event(self, ev)

    @event_handler
    def on_player_leaf_decay(self, ev: LeavesDecayEvent):
        handle_leaves_decay_event(self, ev)

    @event_handler
    def on_player_gamemode(self, ev: PlayerGameModeChangeEvent):
        handle_gamemode_event(self, ev)

    @event_handler
    def on_player_interact(self, ev: PlayerInteractActorEvent):
        handle_interact_event(self, ev)

    @event_handler()
    def on_packet_send(self, ev: PacketSendEvent):
        handle_packetsend_event(self, ev)

    @event_handler()
    def on_packet_receive(self, ev: PacketReceiveEvent):
        handle_packetreceive_event(self, ev)

    @event_handler()
    def on_item_use(self, ev: PlayerItemConsumeEvent):
        handle_item_use(self, ev)

    @event_handler()
    def on_item_pickup(self, ev: PlayerPickupItemEvent):
        handle_item_pickup_event(self, ev)

    @event_handler()
    def on_item_drop(self, ev: PlayerDropItemEvent):
        handle_item_drop_event(self, ev)

    @event_handler()
    def on_entity_hurt(self, ev: ActorDamageEvent):
        handle_damage_event(self, ev)

    @event_handler()
    def on_entity_kb(self, ev: ActorKnockbackEvent):
        handle_kb_event(self, ev)

    @event_handler()
    def on_player_login(self, ev: PlayerLoginEvent):
        handle_login_event(self, ev)

    @event_handler()
    def on_player_join(self, ev: PlayerJoinEvent):
        handle_join_event(self, ev)

    @event_handler()
    def on_player_quit(self, ev: PlayerQuitEvent):
        handle_leave_event(self, ev)

    @event_handler()
    def on_player_kick(self, ev: PlayerKickEvent):
        handle_kick_event(self, ev)

    @event_handler(priority=EventPriority.HIGHEST)
    def on_player_command_preprocess(self, ev: PlayerCommandEvent) -> None:
        handle_command_preprocess(self, ev)

    @event_handler(priority=EventPriority.HIGHEST)
    def on_player_server_command_preprocess(self, ev: ServerCommandEvent) -> None:
        handle_server_command_preprocess(self, ev)

    @event_handler(priority=EventPriority.HIGHEST)
    def on_player_chat(self, ev: PlayerChatEvent):
        handle_chat_event(self, ev)

    @event_handler()
    def on_server_load(self, ev: ServerLoadEvent):
        eco = get_eco_link(self)
        if eco:
            name = perms_util.get_permission_header(eco)
            print(f"[PrimeBDS] Successfully linked economy plugin: {name}")
        else:
            print(f"[PrimeBDS] Did NOT link to an economy plugin... warps will ignore cost")

        self.server.scheduler.run_task(self, perms_util.load_perms(self), 1)
        for player in self.server.online_players:
            self.server.scheduler.run_task(self, self.reload_custom_perms(player), 1)

    def on_load(self):
        plugin_text()

    def on_enable(self):
        self.register_events(self)
        self.db.migrate_table("users", User)
        self.db.migrate_table("mod_logs", ModLog)
        self.serverdb.migrate_table("server_info", ServerData)

        load_config()

        init_jail_intervals(self)
        init_afk_intervals(self)
        last_shutdown_time = self.serverdb.get_server_info().last_shutdown_time
        self.last_shutdown_time = last_shutdown_time 

        if not self.serverdb.get_server_info().allowlist_profile:
            self.serverdb.update_server_info("allowlist_profile", "default")

        self.gamerules = self.serverdb.get_gamerules()
        self.server.scheduler.run_task(self, start_additional_servers(self), 1)
        self.check_for_inactive_sessions()

    def on_disable(self):
        stop_intervals(self)
        clear_all_blockscan_intervals(self)
        clear_all_monitor_intervals(self)
        self.db.close_connection()
        self.sldb.close_connection()

        self.serverdb.update_server_info("last_shutdown_time", int(time.time()))
        self.serverdb.close_connection()

        if not is_nested_multiworld_instance():
            stop_additional_servers(self)
            return

    def check_for_inactive_sessions(self):
        current_time = int(time.time())
        RELOAD_THRESHOLD = 60  # seconds
        MAX_SESSION_LENGTH = 3600  # 1 hour max session length

        was_quick_reload = (
            self.last_shutdown_time
            and current_time - self.last_shutdown_time <= RELOAD_THRESHOLD
        )

        query = "SELECT xuid, name, start_time FROM sessions_log WHERE end_time IS NULL"
        active_sessions = self.sldb.execute(query).fetchall()

        for xuid, player_name, start_time in active_sessions:
            player = self.server.get_player(player_name)
            if not player and not was_quick_reload:
                candidate_end = None
                if self.last_shutdown_time and self.last_shutdown_time > start_time:
                    candidate_end = self.last_shutdown_time
                else:
                    candidate_end = current_time

                max_allowed_end = start_time + MAX_SESSION_LENGTH
                if candidate_end > max_allowed_end:
                    candidate_end = max_allowed_end

                self.sldb.end_session(xuid, candidate_end)

    def reload_custom_perms(self, player: Player):
        user = self.db.get_online_user(player.xuid)
        if not user:
            self.db.save_user(player)
            user = self.db.get_online_user(player.xuid)

        self.vanish_state[player.unique_id] = bool(user.is_vanish) if user else False

        internal_rank = perms_util.check_rank_exists(self, player, user.internal_rank)
        permissions = perms_util.get_rank_permissions(internal_rank)
        user_permissions = self.db.get_permissions(player.xuid)
        managed_perms = perms_util.MANAGED_PERMISSIONS_LIST[:]

        linked_groups = [
            [
                "primebds.command.permban",
                "endstone.command.ban",
            ],
            [
                "primebds.command.ipban",
                "endstone.command.banip"
            ],
            [
                "primebds.command.removeban",
                "endstone.command.unban",
                "endstone.command.unbanip",
            ],
            [
                "primebds.command.filterlist",
                "endstone.command.banlist"
            ]
        ]

        final_permissions = {rperm.lower(): False for rperm in managed_perms}
        for perm, allowed in permissions.items():
            final_permissions[perm.lower()] = allowed
        for perm, allowed in user_permissions.items():
            final_permissions[perm.lower()] = allowed

        linked_permissions = {}
        for group in linked_groups:
            seen_true = False
            seen_false = False

            # Check all permissions in the group for any that already exist
            for perm in group:
                if perm in final_permissions:
                    if final_permissions[perm]:
                        seen_true = True
                    else:
                        seen_false = True

            # Determine what to assign
            if seen_true:
                group_value = True
            elif seen_false:
                group_value = False
            else:
                continue  # none of the group's perms exist yet, skip

            # Apply the decided value to all permissions in the group
            for perm in group:
                final_permissions[perm] = group_value

        for base_perm, linked_list in linked_permissions.items():
            if base_perm in final_permissions:
                base_value = final_permissions[base_perm]
                for linked_perm in linked_list:
                    final_permissions[linked_perm] = base_value

        to_remove = [
            attinfo.attachment for attinfo in player.effective_permissions
            if attinfo.permission == "primebdsoverride"
        ]
        for attachment in to_remove:
            attachment.remove()

        perms_to_apply = list(final_permissions.items())
        attachment = player.add_attachment(self, "primebdsoverride", True)

        plugin_stars = {}
        internal = {"minecraft", "minecraft.command", "endstone", "endstone.command"}

        try:
            for plugin in self.server.plugin_manager.plugins:
                if plugin.is_enabled:
                    for perm, value in perms_to_apply:
                        if perm in internal:
                            continue
                        prefix = perm.split(".")[0]
                        cmd = f"{prefix}.command"
                        if prefix == perm or cmd == perm:
                            plugin_stars[prefix] = value
        except Exception:
            server_registered = {
                str(p.name).lower() for p in self.server.plugin_manager.permissions
            }
            for perm in server_registered:
                if perm in internal:
                    continue
                prefix = perm.split(".")[0]
                cmd = f"{prefix}.command"
                if prefix == perm or cmd == perm:
                    plugin_stars[prefix] = value

        for perm, value in perms_to_apply:
            if perm in internal:
                continue

            prefix = perm.split(".")[0]
            if prefix in plugin_stars:
                attachment.set_permission(perm, plugin_stars[prefix])
            else:
                attachment.set_permission(perm, value)

        if (
            user.internal_rank.lower() == "operator"
            and not player.is_op
            and player.is_valid
        ):
            self.server.dispatch_command(self.server.command_sender, f'op "{user.name}"')
        elif (
            user.internal_rank.lower() != "operator"
            and player.is_op
            and player.is_valid
        ):
            self.server.dispatch_command(self.server.command_sender, f'deop "{user.name}"')

        player.update_commands()
        player.recalculate_permissions()
        perms_util.clear_prefix_suffix_cache()
        perms_util.invalidate_perm_cache(player.xuid)

    def on_command(self, sender: CommandSender, command: Command, args: list[str]) -> bool:
        """Handle incoming commands dynamically"""
        try:
            if command.name in self.handlers:
                if any(arg.find("@e") != -1 or arg.find("@n") != -1 for arg in args):
                    sender.send_message("§cSelector must be player-type")
                    return False
                else:
                    handler_func = self.handlers[command.name]
                    return handler_func(self, sender, args)
            else:
                sender.send_message(f"Command '{command.name}' not found")
                return False

        except Exception as e:
            def clean_traceback(tb):
                cleaned_lines = []
                for line in tb.splitlines():
                    if 'File "' in line:
                        path_start = line.find('"') + 1
                        path_end = line.find('"', path_start)
                        file_path = line[path_start:path_end]
                        hidden_path = os.path.basename(file_path)
                        line = line.replace(file_path, f"<hidden>/{hidden_path}")
                    cleaned_lines.append(line)
                return "\n".join(cleaned_lines)

            error_message = (
                f"§c========\n"
                f"§6This command generated an error -> please report this on our GitHub and provide a copy of the error below!\n"
                f"§c========\n\n"
                f"§e{e}\n\n"
                f"§eCommand Usage: §b{command.name} + {args}\n\n"
                + f"§e{clean_traceback(traceback.format_exc())}\n"
                  f"§r"
            )
            error_message_console = (
                f"========\n"
                f"This command generated an error -> please report this on our GitHub and provide a copy of the error below!\n"
                f"========\n\n"
                f"{e}\n\n"
                f"Command Usage: {command.name} + {args}\n\n"
                + clean_traceback(traceback.format_exc())
            )

            sender.send_message(error_message)

            if sender.name != "Server":
                print(error_message_console)

            return False