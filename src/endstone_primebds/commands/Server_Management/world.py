from endstone import Player
from endstone.command import CommandSender
import psutil
from endstone_primebds.utils.commandUtil import create_command
from endstone_primebds.utils.configUtil import load_config
from endstone_primebds.utils.prefixUtil import infoLog, errorLog

from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from endstone_primebds.primebds import PrimeBDS

# Register command
command, permission = create_command(
    "world",
    "Manages PrimeBDS multiworld!",
    [
        "/world (create|delete|load|unload)<world_subaction: world_subaction> <world_name: string>",
        "/world (cmd)<world_command: world_command> <world_name: string> <command: string>",
        "/world (transfer)<world_transfer: world_transfer> <world_name: string> <player: player>",
        "/world (list)<world_list: world_list>"
     ],
    ["primebds.command.world"]
)

# WORLD COMMAND FUNCTIONALITY
def handler(self: "PrimeBDS", sender: CommandSender, args: list[str]) -> bool:
    def send_feedback(message: str): # TO BE USED IN FUTURE CMD IMPLEMENTATIONS AND REFACTORS
        if isinstance(sender, Player):
            sender.send_message(infoLog() + message.replace("[PrimeBDS]", "").strip())
        else:
            print(message)

    subaction = args[0].lower() if len(args) > 0 else None
    world_name = args[1] if len(args) > 1 else None

    if subaction == "cmd":
        if len(args) < 3:
            send_feedback("[PrimeBDS] Usage: /world <world_name> cmd <command>")
            return False

        command_to_run = " ".join(args[2:])

        # If targeting the main server
        if world_name == self.server.level.name:
            try:
                self.server.dispatch_command(self.server.command_sender, command_to_run)
                send_feedback(f"[PrimeBDS] Command executed on primary world '{world_name}': {command_to_run}")
                return True
            except Exception as e:
                send_feedback(f"[PrimeBDS] Failed to execute command on primary world '{world_name}': {e}")
                return False

        # For additional servers via HTTP
        if world_name not in self.multiworld_ports:
            send_feedback(f"[PrimeBDS] World '{world_name}' is not loaded or registered.")
            return False

        try:
            self.multiworld_http_client.send_command(world_name, command_to_run)
            send_feedback(f"[PrimeBDS] Command sent to world '{world_name}': {command_to_run}")
            return True
        except Exception as e:
            send_feedback(f"[PrimeBDS] Failed to send command to world '{world_name}': {e}")
            return False

    elif subaction == "list":
        config = load_config()
        multiworld = config["modules"].get("multiworld", {})
        worlds = multiworld.get("worlds", {})
        main_ip = multiworld.get("main_ip", "127.0.0.1")
        main_port = self.server.port

        loaded_worlds = list(self.multiworld_processes.keys())
        current_world = self.server.level.name

        if not loaded_worlds:
            send_feedback("[PrimeBDS] No additional worlds are currently loaded.")
        else:
            lines = []
            # Include main world first
            lines.append(f"§8- §r{current_world} §o§8({main_ip}:{main_port}) §r§a[current]§r")
            for lw in loaded_worlds:
                # Find IP and port for each loaded world from config
                target_world = worlds.get(lw, {})
                ip = target_world.get("ip", main_ip)
                port = target_world.get("server-port", 19132)
                lines.append(f"§8- §r{lw} §o§8({ip}:{port})§r")

            send_feedback("[PrimeBDS] Loaded worlds:\n" + "\n".join(lines))
        return True

    elif subaction == "transfer":
        if len(args) < 3:
            send_feedback("[PrimeBDS] Usage: /world <world_name> transfer <player>")
            return False

        player_name = args[2]
        player = self.server.get_player(player_name)
        if player is None:
            send_feedback(f"[PrimeBDS] Player '{player_name}' not found.")
            return False

        config = load_config()
        multiworld = config["modules"].get("multiworld", {})
        worlds = multiworld.get("worlds", {})
        main_ip = multiworld.get("main_ip", "127.0.0.1")
        main_port = self.server.port

        if world_name == self.server.level.name:
            ip = main_ip
            port = main_port
        else:
            target_world = worlds.get(world_name)
            if not target_world:
                # Try to find by level-name fallback
                for key, data in worlds.items():
                    if data.get("level-name") == world_name:
                        target_world = data
                        break

            if not target_world:
                send_feedback(f"[PrimeBDS] World '{world_name}' not found in configuration.")
                return False

            ip = target_world.get("ip", main_ip)
            port = target_world.get("server-port", 19132)

        try:
            player.transfer(ip, port)
            send_feedback(f"[PrimeBDS] Transferred player '{player_name}' to world '{world_name}' ({ip}:{port}).")
            return True
        except Exception as e:
            send_feedback(f"[PrimeBDS] Failed to transfer player '{player_name}': {e}")
            return False

    else:
        send_feedback(f"[PrimeBDS] Unknown subaction '{subaction}'.")
        return False
