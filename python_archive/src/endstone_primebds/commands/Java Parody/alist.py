import os
import json
import shutil

from endstone import Player
from endstone_primebds.utils.config_util import load_config, save_config
from endstone.command import CommandSender
try:
    from endstone.command import BlockCommandSender
except ImportError:
    BlockCommandSender = None 
from endstone_primebds.utils.command_util import create_command

from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from endstone_primebds.primebds import PrimeBDS

# Register command
command, permission = create_command(
    "alist",
    "Manages server allowlist profiles and server allowlist!",
    [
        "/alist (list|check|profiles)<allowlist_sub: allowlist_list>",
        "/alist (add|remove)<allowlist_sub: allowlist_sub_action> <player: string> [ignore_max_player_limit: bool]",
        "/alist (create|use|delete|clear)<allowlist: allowlist_action> <name: string>",
        "/alist (inherit)<allowlist: allowlist_inherit> <child_list: string> <parent_list: string>"
    ],
    ["primebds.command.alist"],
    "op",
    ["wlist"]
)

# ALLOWLIST COMMAND FUNCTIONALITY
def handler(self: "PrimeBDS", sender: CommandSender, args: list[str]) -> bool:
    if BlockCommandSender is not None and isinstance(sender, BlockCommandSender):
       sender.send_message("§cThis command cannot be automated")
       return False

    if any("@" in arg for arg in args):
        sender.send_message(f"§cTarget selectors are invalid for this command")
        return False

    subcommand = args[0].lower()

    if subcommand == "add":
        if len(args) < 2:
            sender.send_message(f"Usage: /alist add <player> [ignore_max_player]")
            return True

        player_name = args[1]
        ignore_max_player = None

        if len(args) >= 3:
            if args[2].lower() in ["true", "false"]:
                ignore_max_player = args[2].lower() == "true"
            else:
                sender.send_message(f"§rignore_max_player must be 'true' or 'false'")
                return True

        self.server.dispatch_command(self.server.command_sender, f"whitelist add \"{player_name}\"")
        sender.send_message(f"§rAdded §b{player_name}§r to allowlist.")

        if ignore_max_player is not None:
            allowlist_path = get_allowlist_path()
            try:
                with open(allowlist_path, 'r') as f:
                    data = json.load(f)

                modified = False
                for entry in data:
                    if entry.get("name") == player_name:
                        
                        user = self.db.get_offline_user(player_name)
                        player = self.server.get_player(player_name)

                        xuid = None
                        if player:
                            xuid = player.xuid
                        elif user is not None:
                            xuid = user.xuid

                        if xuid:
                            entry["ignoresPlayerLimit"] = ignore_max_player
                            entry["xuid"] = xuid
                            modified = True
                        else:
                            sender.send_message(f"§rThe player §b{player_name}§r does not have recorded xuid so the ignore_max_players variable could not be set")
                        break

                if modified:
                    with open(allowlist_path, 'w') as f:
                        json.dump(data, f, indent=4)
                    print(f"[PrimeBDS] Updated 'ignoresPlayerLimit' for {player_name} to {ignore_max_player}")
                else:
                    print(f"[PrimeBDS] Could not find {player_name} in allowlist.json")

            except Exception as e:
                print(f"[PrimeBDS] Failed to modify allowlist: {e}")

        self.server.dispatch_command(self.server.command_sender, f"whitelist reload")

    elif subcommand == "remove":
        if len(args) < 2:
            sender.send_message(f"Usage: /alist remove <player>")
            return True
        player_name = args[1]
        self.server.dispatch_command(self.server.command_sender, f"whitelist remove \"{player_name}\"")
        sender.send_message(f"§rRemoved §b{player_name}§r from allowlist.")
        self.server.dispatch_command(self.server.command_sender, f"whitelist reload")
        return True

    elif subcommand == "list":
        allowlist_path = get_allowlist_path()
        if not os.path.exists(allowlist_path):
            sender.send_message(f"§rAllowlist file not found.")
            return True
        try:
            with open(allowlist_path, 'r') as f:
                data = json.load(f)
            if not data:
                sender.send_message(f"§rAllowlist is empty.")
                return True
            lines = []
            for entry in data:
                name = entry.get("name", "[unknown]")
                ignores = entry.get("ignoresPlayerLimit", False)
                formatted = f"§7{name}"
                if ignores:
                    formatted = f"§6{name} §8(ignores player limit)"
                lines.append(formatted)
            sender.send_message(f"§rAllowlist players:\n" + "\n".join(f"§7- {line}" for line in lines))
        except Exception as e:
            sender.send_message(f"§rFailed to read allowlist: {e}")
        return True

    elif subcommand == "clear":
        target_profile = args[1].strip()
        profiles_dir = get_allowlist_profiles_folder()
        target_path = os.path.join(profiles_dir, f"{target_profile}.json")

        if not os.path.exists(target_path):
            sender.send_message(f"Profile '{target_profile}' does not exist.")
            return True

        try:
            with open(target_path, "r") as f:
                data = json.load(f)

            if not data:
                sender.send_message(f"Profile '{target_profile}' is already empty.")
                return True

            for entry in data:
                player_name = entry.get("name")
                if player_name:
                    self.server.dispatch_command(
                        self.server.command_sender,
                        f'whitelist remove "{player_name}"'
                    )

            with open(target_path, "w") as f:
                json.dump([], f, indent=4)

            current_dir = os.path.dirname(os.path.abspath(__file__))
            while not (
                os.path.exists(os.path.join(current_dir, 'plugins')) and
                os.path.exists(os.path.join(current_dir, 'worlds'))
            ):
                current_dir = os.path.dirname(current_dir)

            current_allowlist = os.path.join(current_dir, "allowlist.json")
            current_profile = self.serverdb.get_server_info().allowlist_profile
            if current_profile == target_profile:
                with open(current_allowlist, "w") as f:
                    json.dump([], f, indent=4)
                self.server.dispatch_command(self.server.command_sender, "whitelist reload")
                sender.send_message(f"Cleared and reloaded active profile '{target_profile}'")
            else:
                sender.send_message(f"Cleared profile '{target_profile}'")

        except Exception as e:
            sender.send_message(f"Failed to clear profile '{target_profile}': {e}")

        return True

    elif subcommand == "check":

        try:

            config = load_config()
            profile_name = self.serverdb.get_server_info().allowlist_profile
            profile_dir = get_allowlist_profiles_folder()
            profile_file = os.path.join(profile_dir, f"{profile_name}.json")

            if not os.path.exists(profile_file):
                sender.send_message(f"§rProfile §c'{profile_name}'§r not found in allowlist_profiles.")
                return True

            # Get actual active allowlist
            current_dir = os.path.dirname(os.path.abspath(__file__))

            while not (
                    os.path.exists(os.path.join(current_dir, 'plugins')) and
                    os.path.exists(os.path.join(current_dir, 'worlds'))
            ):
                current_dir = os.path.dirname(current_dir)

            allowlist_path = os.path.join(current_dir, 'allowlist.json')

            if not os.path.exists(allowlist_path):
                sender.send_message(f"Active allowlist.json file not found.")
                return True

            with open(allowlist_path, 'r') as f:
                active_data = json.load(f)

            active_count = len(active_data)
            sender.send_message(
                f"§rUsing allowlist profile: §b{profile_name}§r with §a{active_count}§r active players.")

        except Exception as e:
            sender.send_message(f"Failed to check allowlist profile: {e}")

        return True

    elif subcommand == "create":
        if len(args) < 2:
            sender.send_message(f"Usage: /alist create <name>")
            return True

        profile_name = args[1].strip()
        path = get_allowlist_profile_path(profile_name)

        if os.path.exists(path):
            sender.send_message(f"Allowlist profile '{profile_name}' already exists.")
            return True

        try:
            with open(path, "w") as f:
                json.dump([], f, indent=4)
            sender.send_message(f"Created allowlist profile '{profile_name}'.")
        except Exception as e:
            sender.send_message(f"Failed to create profile: {e}")
        return True

    elif subcommand == "delete":
        if len(args) < 2:
            sender.send_message(f"Usage: /alist delete <name>")
            return True

        profile_name = args[1].strip()
        path = get_allowlist_profile_path(profile_name)

        if not os.path.exists(path):
            sender.send_message(f"Allowlist profile '{profile_name}' does not exist.")
            return True

        try:
            os.remove(path)
            sender.send_message(f"Deleted allowlist profile '{profile_name}'.")
        except Exception as e:
            sender.send_message(f"Failed to delete profile: {e}")
        return True

    elif subcommand == "use":
        if len(args) < 2:
            sender.send_message(f"Usage: /alist use <profile>")
            return True

        target_profile = args[1].strip()
        profiles_dir = get_allowlist_profiles_folder()
        target_path = os.path.join(profiles_dir, f"{target_profile}.json")

        if not os.path.exists(target_path):
            sender.send_message(f"Profile '{target_profile}' does not exist.")
            return True

        current_dir = os.path.dirname(os.path.abspath(__file__))
        while not (
            os.path.exists(os.path.join(current_dir, 'plugins')) and
            os.path.exists(os.path.join(current_dir, 'worlds'))
        ):
            current_dir = os.path.dirname(current_dir)

        current_allowlist = os.path.join(current_dir, "allowlist.json")
        config = load_config()
        current_profile = self.serverdb.get_server_info().allowlist_profile
        current_profile_path = os.path.join(profiles_dir, f"{current_profile}.json")

        if os.path.exists(current_allowlist):
            with open(current_allowlist, "r") as f:
                current_data = f.read()
            if not os.path.exists(current_profile_path) or open(current_profile_path, "r").read() != current_data:
                with open(current_profile_path, "w") as f:
                    f.write(current_data)
                print(f"[PrimeBDS] Backed up allowlist.json to profile '{current_profile}'")

        def apply_profile():
            shutil.copyfile(target_path, current_allowlist)
            self.serverdb.update_server_info("allowlist_profile", target_profile)
            save_config(config)
            self.server.dispatch_command(self.server.command_sender, "whitelist reload")

            msg = f"Activated allowlist profile '{target_profile}'"
            if isinstance(sender, Player):
                sender.send_message(msg)
            else:
                print(msg)

        self.server.scheduler.run_task(self, apply_profile)
        sender.send_message(f"Allowlist profile will switch to '{target_profile}' shortly")

    elif subcommand == "profiles":
        profiles_dir = get_allowlist_profiles_folder()
        if not os.path.exists(profiles_dir):
            sender.send_message(f"No profiles directory found.")
            return True

        try:
            profiles = [f[:-5] for f in os.listdir(profiles_dir) if f.endswith(".json")]
            if not profiles:
                sender.send_message(f"No saved allowlist profiles.")
                return True

            current_profile = self.serverdb.get_server_info().allowlist_profile

            lines = []
            for profile in profiles:
                if profile == current_profile:
                    lines.append(f"§a{profile} §7(current)")
                else:
                    lines.append(f"§7{profile}")
            sender.send_message(f"Available profiles:\n" + "\n".join(f"§8- {line}" for line in lines))

        except Exception as e:
            sender.send_message(f"Failed to list profiles: {e}")
        return True

    elif subcommand == "inherit":
        child_name = args[1].strip()
        parent_name = args[2].strip()

        profiles_dir = get_allowlist_profiles_folder()
        child_path = os.path.join(profiles_dir, f"{child_name}.json")
        parent_path = os.path.join(profiles_dir, f"{parent_name}.json")

        if not os.path.exists(child_path):
            sender.send_message(f"Child profile '{child_name}' does not exist.")
            return True
        if not os.path.exists(parent_path):
            sender.send_message(f"Parent profile '{parent_name}' does not exist.")
            return True

        try:
            with open(parent_path, "r") as f:
                parent_data = json.load(f)
            with open(child_path, "r") as f:
                child_data = json.load(f)

            child_names = {entry.get("name") for entry in child_data}
            merged_data = child_data[:]
            for entry in parent_data:
                if entry.get("name") not in child_names:
                    merged_data.append(entry)

            with open(child_path, "w") as f:
                json.dump(merged_data, f, indent=4)

            sender.send_message(f"Inherited {len(parent_data)} entries from '{parent_name}' into '{child_name}'")

            current_profile = self.serverdb.get_server_info().allowlist_profile
            if current_profile == child_name:
                current_dir = os.path.dirname(os.path.abspath(__file__))
                while not (
                    os.path.exists(os.path.join(current_dir, 'plugins')) and
                    os.path.exists(os.path.join(current_dir, 'worlds'))
                ):
                    current_dir = os.path.dirname(current_dir)

                current_allowlist = os.path.join(current_dir, "allowlist.json")
                with open(current_allowlist, "w") as f:
                    json.dump(merged_data, f, indent=4)

                backup_path = os.path.join(profiles_dir, f"{child_name}.json")
                with open(backup_path, "w") as f:
                    json.dump(merged_data, f, indent=4)

                self.server.dispatch_command(self.server.command_sender, "whitelist reload")
                sender.send_message(f"Updated live allowlist.json and reloaded whitelist for active profile '{child_name}'")

        except Exception as e:
            sender.send_message(f"Failed to inherit profile: {e}")

    return True

def get_allowlist_profile_path(profile_name: str) -> str:
    return os.path.join(get_allowlist_profiles_folder(), f"{profile_name}.json")

def get_allowlist_path():
    current_dir = os.path.dirname(os.path.abspath(__file__))
    while not (
        os.path.exists(os.path.join(current_dir, 'plugins')) and
        os.path.exists(os.path.join(current_dir, 'worlds'))
    ):
        current_dir = os.path.dirname(current_dir)
    return os.path.join(current_dir, 'allowlist.json')

def get_primebds_data_folder() -> str:
    current_dir = os.path.dirname(os.path.abspath(__file__))
    while not (
        os.path.exists(os.path.join(current_dir, 'plugins')) and
        os.path.exists(os.path.join(current_dir, 'worlds'))
    ):
        current_dir = os.path.dirname(current_dir)
    data_folder = os.path.join(current_dir, 'plugins', 'primebds_data')
    os.makedirs(data_folder, exist_ok=True)
    return data_folder

def get_allowlist_profiles_folder() -> str:
    current_dir = os.path.dirname(os.path.abspath(__file__))
    while not (
        os.path.exists(os.path.join(current_dir, 'plugins')) and
        os.path.exists(os.path.join(current_dir, 'worlds'))
    ):
        current_dir = os.path.dirname(current_dir)

    data_folder = os.path.join(current_dir, 'plugins', 'primebds_data')
    os.makedirs(data_folder, exist_ok=True)

    profiles_folder = os.path.join(data_folder, "allowlist_profiles")
    os.makedirs(profiles_folder, exist_ok=True)

    # Check for existing 'default.json'
    default_profile = os.path.join(profiles_folder, "default.json")
    original_allowlist = os.path.join(current_dir, "allowlist.json")

    if not os.path.exists(default_profile) and os.path.exists(original_allowlist):
        try:
            shutil.copyfile(original_allowlist, default_profile)
            print("[PrimeBDS] Initialized allowlist_profiles/default.json from existing allowlist.json")
        except Exception as e:
            print(f"[PrimeBDS] Failed to create default allowlist profile: {e}")

    return profiles_folder