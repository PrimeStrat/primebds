import os
import shutil
import socket
import subprocess
import sys
import threading
import time

import psutil
from endstone_primebds.utils.config_util import find_and_load_config, load_config, save_properties_file, parse_properties_file

from typing import TYPE_CHECKING
if TYPE_CHECKING:
    from endstone_primebds.primebds import PrimeBDS

def start_additional_servers(self: "PrimeBDS"):
    config = load_config()
    multiworld = config["modules"].get("multiworld", {})
    worlds = {name: cfg for name, cfg in multiworld.get("worlds", {}).items() if cfg.get("enabled", False)}

    default_path = os.path.join("endstone_primebds", "utils", "default_server.properties")
    self.default_props = {}
    if os.path.isfile(default_path):
        self.default_props = parse_properties_file(default_path)

    current_dir = os.path.dirname(os.path.abspath(__file__))
    while not (os.path.exists(os.path.join(current_dir, 'plugins')) and os.path.exists(os.path.join(current_dir, 'worlds'))):
        parent_dir = os.path.dirname(current_dir)
        if parent_dir == current_dir:
            print("[PrimeBDS] Could not locate project root containing 'plugins' and 'worlds'.")
            return
        current_dir = parent_dir

    db_folder = os.path.join(current_dir, 'plugins', 'primebds_data')
    self.multiworld_base_dir = os.path.join(db_folder, "multiworld")
    self.root_plugins_dir = os.path.join(current_dir, "plugins")
    self.seen_level_names = {}
    self.multiworld_lock = threading.Lock()

    os.makedirs(db_folder, exist_ok=True)
    os.makedirs(self.multiworld_base_dir, exist_ok=True)

    if not worlds:
        return

    for world_key, settings in worlds.items():
        threading.Thread(
            target=start_world,
            args=(self, world_key, settings),
            daemon=True
        ).start()

def stop_additional_servers(self: "PrimeBDS"):
    threads = []

    # FIX: Changed from .items() to .keys() to get level names, not tuples
    for level_name in list(self.multiworld_processes.keys()):
        thread = threading.Thread(target=stop_world, args=(self, level_name))
        thread.start()
        threads.append(thread)

    for thread in threads:
        thread.join(timeout=10)  # Add timeout to prevent infinite hanging

    self.multiworld_processes.clear()
    
def start_world(self: "PrimeBDS", world_key: str, settings: dict):
    """Start a single configured world."""
    if world_key == self.server.level.name:
        return

    world_dir = os.path.join(self.multiworld_base_dir, world_key)
    os.makedirs(world_dir, exist_ok=True)
    stop_world(self, world_key)

    world_plugins_dir = os.path.join(world_dir, "plugins")
    if not copy_plugins_to_world(self.root_plugins_dir, world_plugins_dir):
        print(f"[PrimeBDS] Warning: Not all plugins copied for world '{world_key}'")

    server_properties_path = os.path.join(world_dir, "server.properties")
    merged_props = {**self.default_props, **settings}
    save_properties_file(server_properties_path, merged_props)

    level_name = settings.get("level-name", world_key)
    with self.multiworld_lock:
        if level_name in self.seen_level_names:
            self.seen_level_names[level_name] += 1
            level_name = f"{level_name}_{self.seen_level_names[level_name]}"
        else:
            self.seen_level_names[level_name] = 0

    process = launch_endstone_server(self.multiworld_base_dir, world_key, level_name, max_retries=3)
    if not process:
        return

    with self.multiworld_lock:
        self.multiworld_processes[level_name] = process

    try:
        port = int(settings.get("server-port"))
    except (TypeError, ValueError):
        port = self.base_port + len(self.multiworld_processes)
        print(f"[PrimeBDS] Invalid or missing port for '{level_name}', using fallback: {port}")

    with self.multiworld_lock:
        self.multiworld_ports[level_name] = port

    for stream, prefix in [
        (process.stdout, f"[{level_name}] "),
        (process.stderr, f"[{level_name}][ERR] ")
    ]:
        threading.Thread(
            target=forward_output,
            args=(stream, prefix),
            daemon=True
        ).start()

def stop_world(self, world_key: str):
    """Stop a single world by name, including orphan processes in its folder."""
    to_stop = []

    # FIX: Collect level names to stop first (avoid holding lock during iteration)
    with self.multiworld_lock:
        for level_name, proc in list(self.multiworld_processes.items()):
            if level_name.startswith(world_key):
                self.seen_level_names.pop(world_key, None)
                to_stop.append((level_name, proc))

    # FIX: Stop processes outside the lock to prevent deadlocks
    for level_name, proc in to_stop:
        if proc is None:
            continue

        print(f"[PrimeBDS] Stopping world '{level_name}'")
        try:
            if proc.stdin:
                try:
                    hostname = socket.gethostname()
                    ip = socket.gethostbyname(hostname)
                    start_path = os.path.dirname(os.path.abspath(__file__))
                    server_properties = find_and_load_config("server.properties", start_path)
                    port = int(server_properties.get("server-port", 19132))
                    
                    # FIX: Add timeout and error handling for stdin operations
                    proc.stdin.write(f"send @a {ip} {port}\n")
                    proc.stdin.write("stop\n")
                    proc.stdin.flush()
                except (BrokenPipeError, OSError) as e:
                    print(f"[PrimeBDS] Could not send commands to '{level_name}': {e}")
                except Exception as e:
                    print(f"[PrimeBDS] Error writing to stdin for '{level_name}': {e}")
                    
            try:
                proc.wait(timeout=5)
                print(f"[PrimeBDS] Process for '{level_name}' stopped gracefully.")
            except subprocess.TimeoutExpired:
                print(f"[PrimeBDS] Process for '{level_name}' did not stop in time, killing...")
                proc.kill()
                proc.wait(timeout=2)
                print(f"[PrimeBDS] Process for '{level_name}' killed.")
        except Exception as e:
            print(f"[PrimeBDS] Error stopping process for '{level_name}': {e}")

        # FIX: Remove from dict after processing (avoid lock contention)
        with self.multiworld_lock:
            self.multiworld_processes.pop(level_name, None)
            self.multiworld_ports.pop(level_name, None)

    # If no processes were stopped, check for orphan processes
    if not to_stop:
        world_dir = os.path.join(self.multiworld_base_dir, world_key)
        if os.path.exists(world_dir):
            for proc in psutil.process_iter(['pid', 'cwd', 'name']):
                try:
                    if proc.info['cwd'] and os.path.normpath(proc.info['cwd']) == os.path.normpath(world_dir):
                        proc.kill()
                except (psutil.NoSuchProcess, psutil.AccessDenied):
                    continue

def is_nested_multiworld_instance():
    return "plugins{}primebds_data{}multiworld".format(os.sep, os.sep) in os.path.abspath(__file__)

def launch_endstone_server(multiworld_base_dir: str, folder: str, level_name: str, max_retries: int = 3):
    """Launch an Endstone server with retry logic."""
    for attempt in range(max_retries + 1):
        process = subprocess.Popen(
            [sys.executable, "-m", "endstone", "--yes", f"--server-folder={folder}"],
            cwd=multiworld_base_dir,
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )

        time.sleep(1)

        if process.poll() is None:
            return process

        print(f"[PrimeBDS] World '{level_name}' crashed or exited early (attempt {attempt + 1}).")

    print(f"[PrimeBDS] World '{level_name}' failed to start after {max_retries + 1} attempts.")
    return None

def forward_output(stream, prefix):
    while True:
        try:
            line = stream.readline()
            if not line:
                break
            print(f"{prefix}{line}", end='')
        except Exception as e:
            print(f"{prefix}[ReadError]: {e}")
            break

def copy_plugins_to_world(root_plugins_dir, world_plugins_dir, timeout=5):
    os.makedirs(world_plugins_dir, exist_ok=True)

    expected_plugins = []
    for item in os.listdir(root_plugins_dir):
        if not item.endswith(".whl"):
            continue

        source_path = os.path.join(root_plugins_dir, item)
        target_path = os.path.join(world_plugins_dir, item)

        try:
            if os.path.exists(target_path):
                os.remove(target_path)

            shutil.copy2(source_path, target_path)
            expected_plugins.append(item)
        except Exception as e:
            print(f"[PrimeBDS] Failed to copy '{item}': {e}")

    start = time.time()
    while time.time() - start < timeout:
        existing = [f for f in os.listdir(world_plugins_dir) if f.endswith(".whl")]
        if all(plugin in existing for plugin in expected_plugins):
            return True
        time.sleep(0.05)

    return False