import json
import os
import glob
import platform

from endstone import Player
from endstone.command import CommandSender
from endstone_primebds.utils.command_util import create_command

from endstone_primebds.utils.form_wrapper_util import (
    ActionFormData,
    ActionFormResponse,
)
from typing import TYPE_CHECKING
from datetime import datetime

from endstone_primebds.utils.time_util import TimezoneUtils

if TYPE_CHECKING:
    from endstone_primebds.primebds import PrimeBDS

# Register command
command, permission = create_command(
    "viewscriptprofiles",
    "An in-game script profile viewer!",
    ["/viewscriptprofiles"],
    ["primebds.command.vsp"],
    "op",
    ["vsp"]
)

# VSP COMMAND FUNCTIONALITY
def handler(self: "PrimeBDS", sender: CommandSender, args: list[str]) -> bool:
    if isinstance(sender, Player):
        open_profiles_menu(sender)  # Call the function to show profiles menu
    else:
        sender.send_error_message("This command can only be executed by a player.")
    return True

# Get file paths based on OS
def get_profiles_directory() -> str:
    if platform.system() == 'Windows':
        base_dir = os.path.expanduser("~")
        return os.path.join(base_dir, "AppData", "Roaming", "logs", "profiles")
    else:
        candidates = [
            "/container/profiles",
            "/home/container/profiles",
            "/srv/container/profiles",
            "/data/container/profiles",
            os.path.join(os.getcwd(), "container", "profiles")  # relative fallback
        ]

        for path in candidates:
            if os.path.isdir(path):
                return path

        # Fallback
        return "/container/profiles"

# List .cpuprofile files sorted by date
def list_profiles() -> list[str]:
    profiles_directory = get_profiles_directory()
    if not os.path.exists(profiles_directory):
        return []

    # Find all .cpuprofile files
    profile_files = glob.glob(os.path.join(profiles_directory, "*.cpuprofile"))
    profile_files.sort(key=lambda x: os.path.getmtime(x), reverse=True)  # Sort by modification time
    return profile_files

def get_profile_metadata(profile_path: str) -> dict:
    """Extract metadata from a .cpuprofile file."""
    metadata = {
        "path": profile_path,
        "size": os.path.getsize(profile_path),
        "records": None,
        "startTime": None,
        "endTime": None
    }

    try:
        with open(profile_path, 'r') as f:
            content = json.load(f)

        if "nodes" in content and isinstance(content["nodes"], list):
            metadata["records"] = len(content["nodes"])

        # Some cpuprofiles include timestamps (depends on the profiler used)
        if "startTime" in content:
            metadata["startTime"] = content["startTime"]
        if "endTime" in content:
            metadata["endTime"] = content["endTime"]

    except Exception:
        pass

    return metadata

def format_size(bytes_val: int) -> str:
    """Convert bytes into a human-readable string."""
    if bytes_val < 1024:
        return f"{bytes_val} B"
    elif bytes_val < 1024 ** 2:
        return f"{bytes_val / 1024:.1f} KB"
    else:
        return f"{bytes_val / (1024 ** 2):.1f} MB"

def open_profiles_menu(sender: Player):
    profile_files = list_profiles()

    if not profile_files:
        sender.send_message("No profile files found.")
        return

    form = ActionFormData()
    form.title("Script Profiles")
    form.body("Select a profile to view:")

    for profile in profile_files:
        timestamp = datetime.fromtimestamp(os.path.getmtime(profile))
        modified_time = TimezoneUtils.convert_to_timezone(timestamp.timestamp(), "EST")

        # Try to count records from JSON if possible
        try:
            with open(profile, "r") as f:
                metadata = get_profile_metadata(profile)
                records = f"{metadata['records']} records" if metadata["records"] else "0 records"
        except Exception:
            records = "?"

        form.button(f"§4{records} records\n§c{modified_time}")

    form.button("Close")

    def submit(player: Player, result: ActionFormResponse):
        if not result.canceled:
            try:
                selected_index = int(result.selection)
                if 0 <= selected_index < len(profile_files):
                    profile_path = profile_files[selected_index]
                    open_profile_text(player, profile_path)
                else:
                    player.send_message("Invalid selection.")
            except ValueError:
                player.send_message("Invalid selection index.")

    form.show(sender).then(lambda player=sender, result=ActionFormResponse: submit(player, result))

def open_profile_text(player: Player, profile_path: str):
    try:
        with open(profile_path, 'r') as file:
            content = file.read()

            metadata = get_profile_metadata(profile_path)

            # Build metadata summary
            size_str = format_size(metadata["size"])
            records_str = f"{metadata['records']} records" if metadata["records"] else "0 records"
            timestamp = datetime.fromtimestamp(os.path.getmtime(profile_path))
            modified_time = TimezoneUtils.convert_to_timezone(timestamp.timestamp(), "EST")

            header_info = (
                f"§6File: §f{os.path.basename(profile_path)}\n"
                f"§6Modified: §f{modified_time}\n"
                f"§6Size: §f{size_str}\n"
                f"§6Data: §f{records_str}\n"
            )

            try:
                content_data = json.loads(content)
                data = generate_readable_text(content_data)
            except json.JSONDecodeError:
                data = content

            body_text = header_info + data

            form = ActionFormData()
            form.title("§eNode Profiling Summary")
            form.body(body_text)
            form.button("Close")

            def submit(player: Player, result: ActionFormResponse):
                return True

            form.show(player).then(lambda player, result=ActionFormResponse: submit(player, result))

    except Exception as e:
        player.send_message(f"Failed to open the profile: {str(e)}")

def generate_readable_text(node_data):
    """Generate a readable text representation of the node data with Minecraft color codes,
       sorted by hit count, and pointing to the specific line of code for each node."""
    output = []
    output.append("\n§cThis is a summary of the profiling data sorted by hit count from highest to lowest.\n")
    if "nodes" in node_data:
        sorted_nodes = sorted(node_data["nodes"], key=lambda node: node.get('hitCount', 0), reverse=True)

        for node in sorted_nodes:
            output.append(parse_node(node))
            output.append("")  # Empty line between nodes

    return "\n".join(output)

def parse_node(node, indent=0):
    """Format all node information into a readable string with Minecraft color codes."""
    output = []
    spacing = " " * indent

    for key, value in node.items():
        if isinstance(value, dict):
            output.append(f"{spacing}§b{key}:")
            output.append(parse_node(value, indent + 2))  # recurse for nested dict
        elif isinstance(value, list):
            output.append(f"{spacing}§b{key}:")
            for i, item in enumerate(value):
                output.append(f"{spacing}  §7- Entry {i}:")
                if isinstance(item, dict):
                    output.append(parse_node(item, indent + 4))
                else:
                    output.append(f"{spacing}    §f{item}")
        else:
            output.append(f"{spacing}§b{key}: §f{value}")

    return "\n".join(output)