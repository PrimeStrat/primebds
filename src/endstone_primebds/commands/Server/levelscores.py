import json
import os
from typing import TYPE_CHECKING, List

from endstone import Player
from endstone.scoreboard import DisplaySlot, ObjectiveSortOrder
from endstone.command import CommandSender
from endstone.scoreboard import Criteria

from endstone_primebds.utils.command_util import create_command


if TYPE_CHECKING:
    from endstone_primebds.primebds import PrimeBDS

# Register command
command, permission = create_command(
    "levelscores",
    "Save and load scoreboard profiles between worlds",
    ["/levelscores (save|load|list|delete)<action: file_action> [name: string]"],
    ["primebds.command.levelscores"]
)

# LEVELDB CMD FUNCTIONALITY
def handler(self: "PrimeBDS", sender: CommandSender, args: list[str]) -> bool:
    player = self.server.get_player(sender.name)

    action = args[0]

    if action == "save" and len(args) > 1:
        name = args[1]
        return save_scoreboard(self, player, name)
    elif action == "load" and len(args) > 1:
        name = args[1]
        return load_scoreboard(self, player, name)
    elif action == "list":
        profiles = list_scoreboard_profiles()
        player.send_message(
            f"Available profiles: {', '.join(profiles)}" if profiles else f"No profiles found.")
        return True
    elif action == "delete" and len(args) > 1:
        name = args[1]
        return delete_scoreboard(self, player, name)

    player.send_message(f"Invalid action. Use 'save', 'load', or 'list'.")
    return False

def save_scoreboard(self: "PrimeBDS", player, name: str) -> bool:
    scoreboard_data_folder = get_scoreboard_directory()
    save_path = os.path.join(scoreboard_data_folder, f"{name}.json")

    objectives = self.server.scoreboard.objectives
    data = {}

    entry_count = len(list(self.server.scoreboard.entries))
    invalid_entry_counts = 0

    for objective in objectives:
        obj_data = {
            "is_fully_loaded": False,
            "display_name": objective.display_name,
            "criteria": objective.criteria.name,
            "render_type": objective.render_type.value,
            "display_slot": str(objective.display_slot) if hasattr(objective.display_slot, '__str__') else str(DisplaySlot.PLAYER_LIST),
            "sort_order": str(objective.sort_order) if hasattr(objective.sort_order, '__str__') else str(ObjectiveSortOrder.ASCENDING),
            "entries": {}
        }

        for entry in list(self.server.scoreboard.entries):
            score = objective.get_score(entry)

            if isinstance(entry, str):
                entry_key = entry
            else:
                if hasattr(entry, "xuid"):
                    entry_key = entry.xuid
                else:
                    entry_count -= 1
                    invalid_entry_counts += 1
                    continue

            if entry_key is not None:
                obj_data["entries"][entry_key] = {
                    "value": score.value,
                    "loaded": False
                }

        data[objective.name] = obj_data

    with open(save_path, 'w') as f:
        json.dump(data, f, indent=4)

    if not isinstance(player, Player):
        print(f"Saved scoreboard profile '{name}'")
        return False
    else:
        player.send_message(f"Saved scoreboard profile '{name}' with {entry_count} entries!")
        if invalid_entry_counts > 0:
            player.send_message(f"{invalid_entry_counts} were removed as mob entries cannot be transferred.")
    return True

def load_scoreboard(self: "PrimeBDS", player, name: str) -> bool:
    scoreboard_data_folder = get_scoreboard_directory()
    load_path = os.path.join(scoreboard_data_folder, f"{name}.json")

    if not os.path.exists(load_path):
        if isinstance(player, Player):
            player.send_message(f"Scoreboard profile '{name}' not found!")
        else:
            print(f"Scoreboard profile '{name}' not found!")
        return False

    with open(load_path, 'r') as f:
        data = json.load(f)

    new_profile = False
    modified = False

    for obj_name, obj_data in data.items():
        objective = self.server.scoreboard.get_objective(obj_name)

        if not objective:
            criteria = Criteria.DUMMY
            objective = self.server.scoreboard.add_objective(obj_name, criteria, obj_data["display_name"])
            new_profile = True

        all_entries_loaded = True

        for entry_name, entry_data in obj_data["entries"].items():
            is_xuid = entry_name.isdigit() and len(entry_name) >= 12

            if is_xuid:
                player_obj = None
                for player in self.server.online_players:
                    if player.xuid == entry_name:
                        player_obj = player
                        break

                if player_obj is None:
                    all_entries_loaded = False
                    continue

                score_key = player_obj

            else:
                score_key = entry_name

            score_obj = objective.get_score(score_key)
            score_obj.value = entry_data["value"]

            if not entry_data.get("loaded", False):
                entry_data["loaded"] = True
                modified = True

        if obj_data.get("is_fully_loaded") != all_entries_loaded:
            obj_data["is_fully_loaded"] = all_entries_loaded
            modified = True

    if modified:
        with open(load_path, 'w') as f:
            json.dump(data, f, indent=4)

    status = "NEW" if new_profile else "OLD"
    if isinstance(player, Player):
        player.send_message(f"Loaded scoreboard profile '{name}' §7§o[{status}]")
    else:
        print(f"Loaded scoreboard profile '{name}' [{status}]")

    return True

def delete_scoreboard(self: "PrimeBDS", player, name: str) -> bool:
    delete_path = os.path.join(get_scoreboard_directory(), f"{name}.json")

    if not os.path.exists(delete_path):
        if isinstance(player, Player):
            player.send_message(f"Scoreboard profile '{name}' not found!")
        else:
            print(f"Scoreboard profile '{name}' not found!")
        return False

    os.remove(delete_path)

    if not isinstance(player, Player):
        print(f"Deleted scoreboard profile '{name}'")
        return False
    else:
        player.send_message(f"Deleted scoreboard profile '{name}'")
    return True

def list_scoreboard_profiles() -> List[str]:
    directory = get_scoreboard_directory()
    return [f[:-5] for f in os.listdir(directory) if f.endswith('.json')]

def get_scoreboard_directory() -> str:
    current_dir = os.path.dirname(os.path.abspath(__file__))
    while not (
        os.path.exists(os.path.join(current_dir, 'plugins')) and
        os.path.exists(os.path.join(current_dir, 'worlds'))
    ):
        current_dir = os.path.dirname(current_dir)

    scoreboard_data_folder = os.path.join(current_dir, 'plugins', 'primebds_data', 'scoreboard_data')
    os.makedirs(scoreboard_data_folder, exist_ok=True)
    return scoreboard_data_folder
