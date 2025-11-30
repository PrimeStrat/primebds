import os
import shutil
import socket
import threading
from endstone import Player
from endstone.command import CommandSender
try:
    from endstone.command import BlockCommandSender
except ImportError:
    BlockCommandSender = None 
from endstone_primebds.handlers.multiworld import start_world, stop_world
from endstone_primebds.utils.command_util import create_command
from endstone_primebds.utils.config_util import find_and_load_config, save_config, load_config
from endstone_primebds.utils.target_selector_util import get_matching_actors
from endstone_primebds.utils.address_util import is_valid_port
from endstone_primebds.utils.form_wrapper_util import ModalFormData, ModalFormResponse

from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from endstone_primebds.primebds import PrimeBDS

# Register command
command, permission = create_command(
    "world",
    "Manages PrimeBDS multiworld!",
    [
        "/world (delete)<world_delete: world_delete> <world_name: string> [erase_files: bool]",
        "/world (create)<world_create: world_create> <world_name: string> <port: int>",
        "/world (config|enable|disable)<world_config: world_config> <world_name: string>",
        "/world (cmd)<world_command: world_command> <world_name: string> <command: string>",
        "/world (send)<world_send: world_send> <world_name: string> <player: player>",
        "/world (list)<world_list: world_list>"
     ],
    ["primebds.command.world"]
)

# WORLD COMMAND FUNCTIONALITY
def handler(self: "PrimeBDS", sender: CommandSender, args: list[str]) -> bool:
    def send_feedback(message: str):
        if isinstance(sender, Player):
            sender.send_message(message.replace("[PrimeBDS]", "").strip())
        else:
            print(message)

    if BlockCommandSender is not None and isinstance(sender, BlockCommandSender):
       sender.send_message("§cThis command cannot be automated")
       return False

    subaction = args[0].lower() if len(args) > 0 else None
    world_name = args[1] if len(args) > 1 else None

    start_path = os.path.dirname(os.path.abspath(__file__))
    config = find_and_load_config("primebds_data/config.json", start_path, "multiworld", 20, True)
    server_properties = find_and_load_config("server.properties", start_path)

    if subaction == "cmd":
        multiworld = config.get("modules", {}).get("multiworld", {})
        worlds = multiworld.get("worlds", {})
        is_enabled = worlds[world_name].get("enabled", False)
        if not is_enabled:
            send_feedback("[PrimeBDS] This world is currently §cDisabled")
            return False

        command_to_run = " ".join(args[2:])

        if server_properties is None:
            send_feedback("[PrimeBDS] Unable to locate properties list")
            return False

        main_level = server_properties.get("level-name", "Unknown")

        if world_name.lower() == main_level.lower():
            try:
                self.server.dispatch_command(self.server.command_sender, command_to_run)
                return True
            except Exception as e:
                send_feedback(f"[PrimeBDS] Failed to execute command on primary world '{world_name}': {e}")
                return False

        if world_name not in self.multiworld_processes:
            if world_name.lower() == main_level.lower():
                send_feedback(f"[PrimeBDS] Commands can only be sent from the main world")
            else:
                send_feedback(f"[PrimeBDS] World '{world_name}' is not loaded or registered")
            return False

        process = self.multiworld_processes[world_name]
        try:
            if process.stdin:
                process.stdin.write(f"{command_to_run}\n")
                process.stdin.flush()
                send_feedback(f"[PrimeBDS] Command sent to world '{world_name}': {command_to_run}")
                return True
            else:
                send_feedback(f"[PrimeBDS] Cannot send command: stdin not available for world '{world_name}'")
                return False
        except Exception as e:
            send_feedback(f"[PrimeBDS] Failed to send command to world '{world_name}': {e}")
            return False

    elif subaction == "list":

        if config is None or server_properties is None:
            send_feedback("[PrimeBDS] Unable to locate config list")
            return False

        main_port = int(server_properties.get("server-port", 19132))
        current_world = self.server.level.name

        multiworld = config["modules"].get("multiworld", {})
        worlds = multiworld.get("worlds", {})

        lines = []

        if server_properties.get("enabled", True):
            lines.append(f"§7- §a[+] §r{current_world} §o§8({main_port}) §r§a[current]§r")

        for world_key, world_data in worlds.items():
            port = world_data.get("server-port", 19132)
            level_name = world_data.get("level-name", world_key)
            is_current = level_name == current_world
            is_enabled = world_data.get("enabled", False)
            current_suffix = " §r§a[current]" if is_current else ""
            enabled_str = f"§a[+]" if is_enabled else f"§c[-]"
            lines.append(f"§7- {enabled_str} §r{level_name} §o§8({port}) {current_suffix}")

        send_feedback("[PrimeBDS] Worlds List:\n" + "\n".join(lines))
        return True

    elif subaction == "send":
        world_name = args[1]
        targets = get_matching_actors(self, args[2], sender)

        if config is None or server_properties is None:
            send_feedback("[PrimeBDS] Unable to locate config list")
            return False

        # Main world IP and port defaults
        main_port = int(server_properties.get("server-port", 19132))
        main_world = server_properties.get("level-name", 19132)

        # Config overrides
        multiworld = config.get("modules", {}).get("multiworld", {})
        worlds = multiworld.get("worlds", {})
        is_enabled = worlds[world_name].get("enabled", False)
        if not is_enabled:
            send_feedback("[PrimeBDS] This world is currently §cDisabled")
            return False

        hostname = socket.gethostname()
        ip = socket.gethostbyname(hostname)

        # Determine target IP/port
        if world_name.lower() == main_world.lower():
            port = main_port
        else:
            target_world = worlds.get(world_name)

            if not target_world:
                # Try to match via level-name if key mismatch
                for key, data in worlds.items():
                    if data.get("level-name").lower() == world_name.lower():
                        target_world = data
                        break

            if not target_world:
                send_feedback(f"[PrimeBDS] World '{world_name.lower()}' not found in configuration")
                return False
            
            port = target_world.get("server-port", 19132)

        for target in targets:
            try:
                target.transfer(ip, port)
                return True
            except Exception as e:
                send_feedback(f"[PrimeBDS] Failed to send player '{target.name}': {e}")
                return False

    elif subaction == "create":
        main_level = server_properties.get("level-name", "Unknown")
        if self.server.level.name != main_level:
            send_feedback("[PrimeBDS] Worlds can only be created from the main world")
            return False

        port = args[2]
        if not is_valid_port(port):
            sender.send_message(f"Invalid port number: {port}")
            return False

        port = int(port)
        config = load_config()
        multiworld = config.setdefault("modules", {}).setdefault("multiworld", {})
        worlds = multiworld.setdefault("worlds", {})

        used_ports = {w.get("server-port") for w in worlds.values()}
        used_ports.update({w.get("server-portv6") for w in worlds.values()})
        main_ipv4 = int(server_properties.get("server-port", 19132))
        main_ipv6 = int(server_properties.get("server-portv6", 19133))
        used_ports.update({main_ipv4, main_ipv6})

        if port in used_ports:
            send_feedback(f"[PrimeBDS] Port {port} is already in use by another world")
            return False

        port_v6 = port + 1
        while port_v6 in used_ports:
            port_v6 += 1
        used_ports.add(port_v6)

        if world_name in worlds:
            send_feedback(f"[PrimeBDS] World '{world_name}' already exists in configuration")
            return False

        level_names = {w.get("level-name", name) for name, w in worlds.items()}
        level_names.add(main_level)
        if world_name in level_names:
            send_feedback(f"[PrimeBDS] Level name '{world_name}' is already in use by another world")
            return False

        current_dir = os.path.dirname(os.path.abspath(__file__))
        while not (os.path.exists(os.path.join(current_dir, 'plugins')) and os.path.exists(os.path.join(current_dir, 'worlds'))):
            parent_dir = os.path.dirname(current_dir)
            if parent_dir == current_dir:
                send_feedback("[PrimeBDS] Could not locate project root containing 'plugins' and 'worlds'")
                return False
            current_dir = parent_dir

        world_props = {
            "enabled": True,
            "linked": False,
            "server-port": port,
            "server-portv6": port_v6,
            "level-name": world_name,
            "server-name": "Dedicated Server",
            "gamemode": "survival",
            "difficulty": "easy",
            "default-player-permission-level": "member",
            "max-players": 10,
            "view-distance": 12,
            "tick-distance": 4,
            "max-threads": 0,
            "level-seed": "",
            "compression-threshold": 30000,
            "texturepack-required": False,
            "allow-list": False,
            "allow-cheats": True
        }

        worlds[world_name] = world_props

        try:
            save_config(config, True)
        except Exception as e:
            send_feedback(f"[PrimeBDS] Failed to save configuration: {e}")
            return False
        
        try:
            start_world(self, world_name, world_props)
        except Exception as e:
            send_feedback(f"[PrimeBDS] Failed to start newly created world: {e}")
            return False

        send_feedback(f"[PrimeBDS] World '{world_name}' registered with PORT {port} (IPv6: {port_v6})")
        return True

    elif subaction == "delete":        
        main_level = server_properties.get("level-name", "Unknown")
        if self.server.level.name != main_level:
            send_feedback(f"[PrimeBDS] Worlds can only be deleted from the main world")
            return False

        erase_files = args[2].lower() == "true" if len(args) > 2 else False

        config = load_config()
        multiworld = config.get("modules", {}).get("multiworld", {})
        worlds = multiworld.get("worlds", {})

        if world_name not in worlds:
            send_feedback(f"[PrimeBDS] World '{world_name}' not found in config")
            return False

        is_enabled = worlds[world_name].get("enabled", False)
        if is_enabled:
            stop_world(self, world_name)
        del worlds[world_name]

        current_dir = os.path.dirname(os.path.abspath(__file__))
        while not (os.path.exists(os.path.join(current_dir, 'plugins')) and os.path.exists(os.path.join(current_dir, 'worlds'))):
            parent_dir = os.path.dirname(current_dir)
            if parent_dir == current_dir:
                send_feedback("[PrimeBDS] Could not locate project root containing 'plugins' and 'worlds'")
                return False
            current_dir = parent_dir
            
        try:
            save_config(config, True)
        except Exception as e:
            send_feedback(f"[PrimeBDS] Failed to update config.json: {e}")
            return False

        if erase_files:
            world_folder = os.path.join(current_dir, "plugins", "primebds_data", "multiworld", world_name)
            try:
                if os.path.isdir(world_folder):
                    shutil.rmtree(world_folder)
                    send_feedback(f"[PrimeBDS] World '{world_name}' files erased from disk")
                else:
                    send_feedback(f"[PrimeBDS] No folder found for world '{world_name}', config entry only deleted")
            except Exception as e:
                send_feedback(f"[PrimeBDS] Failed to delete world files: {e}")
                return False
        else:
            send_feedback(f"[PrimeBDS] World '{world_name}' removed from config only with files kept")

        return True

    elif subaction == "enable":
        main_level = server_properties.get("level-name", "Unknown")
        if world_name == main_level:
            send_feedback(f"[PrimeBDS] You cannot disable the main world")
            return False

        if self.server.level.name != main_level:
            send_feedback(f"[PrimeBDS] Worlds can only be deleted from the main world")
            return False
        
        if is_world_enabled(self, world_name):
            send_feedback(f"[PrimeBDS] The world is currently running")
            return False

        config = load_config()
        multiworld = config.get("modules", {}).get("multiworld", {})
        worlds = multiworld.get("worlds", {})

        if world_name not in worlds:
            send_feedback(f"[PrimeBDS] World '{world_name}' not found in config")
            return False

        is_enabled = worlds[world_name].get("enabled", False)
        if not is_enabled:
            threading.Thread(
                target=start_world,
                args=(self, world_name, worlds[world_name]),
                daemon=True
            ).start()
            send_feedback(f"[PrimeBDS] §e{world_name} §rwas §aenabled")
            worlds[world_name]["enabled"] = True
            save_config(config)

    elif subaction == "disable":
        main_level = server_properties.get("level-name", "Unknown")
        if world_name == main_level:
            send_feedback(f"[PrimeBDS] You cannot disable the main world")
            return False
        elif self.server.level.name != main_level:
            send_feedback(f"[PrimeBDS] Worlds can only be disabled from the main world")
            return False
        
        if not is_world_enabled(self, world_name):
            send_feedback(f"[PrimeBDS] The world is already disabled")
            return False

        config = load_config()
        multiworld = config.get("modules", {}).get("multiworld", {})
        worlds = multiworld.get("worlds", {})

        if world_name not in worlds:
            send_feedback(f"[PrimeBDS] World '{world_name}' not found in config")
            return False

        is_enabled = worlds[world_name].get("enabled", False)
        if is_enabled:
            thread = threading.Thread(target=stop_world, args=(self, world_name))
            thread.start()
            send_feedback(f"[PrimeBDS] §e{world_name} §rwas §cdisabled")
            worlds[world_name]["enabled"] = False
            save_config(config)

    elif subaction == "config":
        multiworld = config.get("modules", {}).get("multiworld", {})
        worlds = multiworld.get("worlds", {})

        if world_name not in worlds:
            send_feedback(f"[PrimeBDS] World '{world_name}' not found in config")
            return False

        settings = worlds[world_name]
        field_map = [(k, type(v)) for k, v in settings.items() if isinstance(v, (str, int, float, bool, list))]

        form = ModalFormData()
        form.title(f"World Configuration: {world_name}")
        for key, value_type in field_map:
            value = settings[key]
            if value_type == bool:
                form.toggle(key, value)
            else:
                form.text_field(key, str(value), str(value))
        form.submit_button("Save Changes")

        def submit_modal(player: Player, response: ModalFormResponse):
            if response.canceled:
                return

            new_values = response.formValues
            updated = {}

            for i, (key, value_type) in enumerate(field_map):
                old_value = settings[key]
                new_value = new_values[i]

                if isinstance(new_value, str):
                    val = new_value.strip().lower()
                    if val in ("true", "false"):
                        new_value = bool(val == "true")

                if value_type == bool:
                    new_value = bool(new_value)
                elif value_type == int:
                    try:
                        new_value = int(new_value)
                    except ValueError:
                        new_value = old_value
                elif value_type == float:
                    try:
                        new_value = float(new_value)
                    except ValueError:
                        new_value = old_value
                elif value_type == list:
                    new_value = [x.strip() for x in str(new_value).split(",") if x.strip()]
                else:
                    new_value = str(new_value)

                if new_value != old_value:
                    updated[key] = new_value
                    settings[key] = new_value

            if not updated:
                return 

            used_ports = {str(wcfg.get("server-port")) for wname, wcfg in worlds.items() if wname != world_name and wcfg.get("server-port") is not None}
            used_names = {wcfg.get("level-name", wname) for wname, wcfg in worlds.items() if wname != world_name}

            new_port = str(settings.get("server-port", ""))
            new_level_name = settings.get("level-name", world_name)

            if new_port in used_ports:
                player.send_message(f"§cPort {new_port} already in use by another world")
                return

            if new_level_name in used_names:
                player.send_message(f"§cLevel name '{new_level_name}' already in use by another world")
                return

            save_config(config, True)
            player.send_message(f"§aUpdated config for world '{world_name}'")

            is_enabled = settings.get("enabled", False)
            if is_enabled:
                restart_form = ModalFormData()
                restart_form.title(f"Restart World '{world_name}'?")
                restart_form.toggle("Restart world now", True)
                restart_form.submit_button("Confirm")

                def handle_restart(player: Player, resp: ModalFormResponse):
                    if resp.canceled:
                        return
                    if bool(resp.formValues[0]):
                        stop_thread = threading.Thread(target=stop_world, args=(self, world_name))
                        stop_thread.start()
                        stop_thread.join()
                        threading.Thread(
                            target=start_world,
                            args=(self, world_name, worlds[world_name]),
                            daemon=True
                        ).start()
                        player.send_message(f"World '{world_name}' restarted successfully")
                    else:
                        player.send_message(f"World '{world_name}' changes saved without restart")
                
                restart_form.show(player).then(lambda resp, pl=player: handle_restart(resp, pl))
            else:
                thread = threading.Thread(target=stop_world, args=(self, world_name))
                thread.start()
                player.send_message(f"World '{world_name}' changes saved")

        form.show(sender).then(lambda resp, pl=sender: submit_modal(resp, pl))

    else:
        send_feedback(f"[PrimeBDS] Unknown subaction '{subaction}'")
        return False

def is_world_enabled(self, world_key: str) -> bool:
    with self.multiworld_lock:
        return any(level_name.startswith(world_key) for level_name in self.multiworld_processes)

