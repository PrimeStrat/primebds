import json
import os
import glob
import platform

from endstone import Player, ColorFormat
from endstone.command import CommandSender
from endstone_primebds.utils.commandUtil import create_command

from endstone_primebds.utils.formWrapperUtil import (
    ActionFormData,
    ActionFormResponse,
)
from typing import TYPE_CHECKING
from datetime import datetime

from endstone_primebds.utils.timeUtil import TimezoneUtils

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

# Display the profile selection menu
def open_profiles_menu(sender: Player):
    profile_files = list_profiles()

    if not profile_files:
        sender.send_message(f"No profile files found.")
        return

    form = ActionFormData()
    form.title("Script Profiles")
    form.body("Select a profile to view:")

    for profile in profile_files:
        timestamp = datetime.fromtimestamp(os.path.getmtime(profile))
        modified_time = TimezoneUtils.convert_to_timezone(timestamp.timestamp(), "EST")
        form.button(f"{ColorFormat.GOLD}CPUPROFILE\n{ColorFormat.RED}{modified_time}")

    form.button(f"Close")

    def submit(player: Player, result: ActionFormResponse):
        if not result.canceled:
            try:
                selected_index = int(result.selection)
                if 0 <= selected_index < len(profile_files):
                    # Load and display the selected profile (assuming read_profile is implemented)
                    profile_path = profile_files[selected_index]
                    open_profile_text(player, profile_path)
                else:
                    player.send_message(f"Invalid selection.")
            except ValueError:
                player.send_message(f"Invalid selection index.")

    form.show(sender).then(lambda player=sender, result=ActionFormResponse: submit(player, result))

def open_profile_text(player: Player, profile_path: str):
    try:
        with open(profile_path, 'r') as file:
            content = file.read()

            try:
                # If the content is JSON-like, use this for conversion
                content_data = json.loads(content)
                data = generate_readable_text(content_data)
            except json.JSONDecodeError:
                # If content is not JSON, just use it as-is
                data = content

            form = ActionFormData()
            form.title("§e--- Node Profiling Summary ---")
            form.body(f"{data}")
            form.button(f"Close")
            form.show(player).then(lambda player, result=ActionFormResponse: submit(player, result))

            def submit(player: Player, result: ActionFormResponse):
                return True

    except Exception as e:
        player.send_message(f"Failed to open the profile: {str(e)}")

def generate_readable_text(node_data):
    """Generate a readable text representation of the node data with Minecraft color codes, sorted by hit count,
       and pointing to the specific line of code for each node."""
    output = []

    # Add a description at the top
    output.append("§7This is a summary of the profiling data sorted by hit count from highest to lowest.\n")
    output.append("§7Each node represents a function or part of the code executed during profiling.\n")
    output.append("§7Generally speaking, the higher the hit count the more performance intensive.\n\n")

    # Handle nodes if they exist and sort them by hitCount in descending order
    if "nodes" in node_data:
        # Sort nodes by hitCount (highest to lowest)
        sorted_nodes = sorted(node_data["nodes"], key=lambda node: node.get('hitCount', 0), reverse=True)

        for node in sorted_nodes:
            # Add formatted node data to output with Minecraft colors
            output.append(parse_node(node))
            output.append("")  # Empty line between nodes

    return "\n".join(output)

def parse_node(node):
    """Format the node information into a human-readable string with Minecraft color codes,
       and point to the specific line of code."""
    # Extract details from the node's call frame
    function_name = node.get('callFrame', {}).get('functionName', 'Unknown Function')
    file_name = node.get('callFrame', {}).get('url', 'Unknown File')
    line_number = node.get('callFrame', {}).get('lineNumber', 'Unknown Line')
    column_number = node.get('callFrame', {}).get('columnNumber', 'Unknown Column')
    hit_count = node.get('hitCount', 'Unknown Hit Count')

    # Format the node details with Minecraft color codes
    node_info = f"§bNode ID: §f{node.get('id', 'N/A')}, §bHit Count: §f{hit_count}, "
    node_info += f"§bFunction: §f{function_name}\n"
    node_info += f"§7File: §f{file_name}, §7Line: §f{line_number}, §7Column: §f{column_number}"

    return node_info