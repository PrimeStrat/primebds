import os
import json
import requests
from endstone import Player
from endstone.command import CommandSender
try:
    from endstone.command import BlockCommandSender
except ImportError:
    BlockCommandSender = None 

import endstone_primebds
from endstone_primebds.utils.config_util import open_text_file
from endstone_primebds.utils.command_util import create_command
from endstone_primebds.utils.form_wrapper_util import ActionFormData, ActionFormResponse

# Register command
command, permission = create_command(
    "updatepacks",
    "Updates the server packs!",
    ["/updatepacks (resource)<target_rp: target_rp> (version|uuid)<update_target>",
     "/updatepacks (behavior)<target_bp: target_bp>"],
    ["primebds.command.reloadpacks"]
)

# RELOADPACKS FUNCTIONALITY
def handler(self, sender: CommandSender, args: list[str]) -> bool:

    if BlockCommandSender is not None and isinstance(sender, BlockCommandSender):
       sender.send_message("§cThis command cannot be automated")
       return False



    if not isinstance(sender, Player) and "behavior" in args:
        sender.send_message(f"You cannot update behavior packs from terminal")
        return False

    current_dir = os.path.dirname(endstone_primebds.__file__)
    while not os.path.exists(os.path.join(current_dir, 'worlds')):
        current_dir = os.path.dirname(current_dir)

    RP_PATH = os.path.join(current_dir, 'worlds', self.server.level.name, 'resource_packs')
    BP_PATH = os.path.join(current_dir, 'worlds', self.server.level.name, 'behavior_packs')

    # Handle Resource Packs
    if "resource" in args:
        if not os.path.exists(RP_PATH):
            sender.send_message("No resource packs found to update")
            return False

        updated_packs = 0
        updated_pack_names = []
        new_world_packs_data = []

        for pack in os.listdir(RP_PATH):
            pack_path = os.path.join(RP_PATH, pack)
            manifest_path = os.path.join(pack_path, "manifest.json")
            if not os.path.isfile(manifest_path):
                continue

            try:
                manifest_content = open_text_file(manifest_path, mode="r")
                manifest = json.loads(manifest_content)

                # Update version
                if args[1] == "version":
                    header_version = manifest.get("header", {}).get("version", [1, 0, 0])
                    header_version[-1] += 1
                    manifest["header"]["version"] = header_version

                # Update UUID and track for world_resource_packs.json
                if args[1] == "uuid":
                    new_uuid = fetch_uuid()
                    if new_uuid:
                        manifest["header"]["uuid"] = new_uuid
                    else:
                        continue
                else:
                    new_uuid = manifest.get("header", {}).get("uuid")

                # Save updated manifest
                manifest_text = json.dumps(manifest, indent=4)
                result = open_text_file(manifest_path, mode="w", text=manifest_text)
                if result is None:
                    print(f"[PrimeBDS] Failed to write manifest to {manifest_path}")

                updated_packs += 1
                updated_pack_names.append(pack)

                new_world_packs_data.append({
                    "pack_id": new_uuid,
                    "version": manifest.get("header", {}).get("version", [1, 0, 0])
                })

            except Exception as e:
                sender.send_message(f"Failed to update {pack}: {e}")
                continue

        world_packs_path = os.path.join(os.path.dirname(RP_PATH), "world_resource_packs.json")
        try:
            world_text = json.dumps(new_world_packs_data, indent=4)
            result = open_text_file(world_packs_path, mode="w", text=world_text)
            if result is None:
                print(f"[PrimeBDS] Failed to write pack data to {world_packs_path}")
        except Exception as e:
            sender.send_message(f"Failed to update world_resource_packs.json: {e}")

        if updated_packs > 0:
            sender.send_message(
                f"Updated {updated_packs} resource pack(s): {', '.join(updated_pack_names)}!\n§7§oREQUIRES SERVER RESTART TO APPLY"
            )
        else:
            sender.send_message("No resource packs were updated.")

    elif "behavior" in args:
        if not os.path.exists(BP_PATH):
            sender.send_message(f"No behavior packs found to update")
            return False
        else:
            select_pack(sender, BP_PATH)
        return True

def fetch_uuid():
    url = "https://www.uuidtools.com/api/generate/v4"
    try:
        response = requests.get(url, timeout=5)
        response.raise_for_status()
        uuid_list = response.json()
        return uuid_list[0] if uuid_list else None
    except Exception as e:
        print(f"UUID fetch failed: {e}")
        return None

def select_pack(sender: CommandSender, path) -> None:
    packs = [pack for pack in os.listdir(path) if os.path.isdir(os.path.join(path, pack))]

    if not packs:
        sender.send_message(f" No behavior packs found.")
        return

    form = ActionFormData()
    form.title("Select Behavior Pack")
    form.body("Choose a behavior pack to update:")

    for pack in packs:
        form.button(pack)

    form.button("Cancel")

    form.show(sender).then(
        lambda player=sender, result=ActionFormResponse: select_dependency(player, result, packs, path)
    )

def select_dependency(player: Player, result: ActionFormResponse, packs, path):
    if result.canceled or int(result.selection) >= len(packs):
        return

    # Get the chosen pack from the list of packs
    chosen_pack = packs[result.selection] if result.selection is not None else None
    if not chosen_pack:
        return

    manifest_path = os.path.join(path, chosen_pack, "manifest.json")

    if not os.path.exists(manifest_path):
        player.send_message(f"No manifest file found for {chosen_pack}.")
        return

    try:
        # Open the manifest file and load its contents
        manifest_content = open_text_file(manifest_path, mode="r")
        manifest = json.loads(manifest_content)

        # Extract dependencies from the manifest
        dependencies = manifest.get("dependencies", [])

        if not dependencies:
            player.send_message(f"No dependencies found for {chosen_pack}.")
            return

        # Create the form to display dependencies
        form = ActionFormData()
        form.title(f"Select Dependency for {chosen_pack}")
        form.body("Choose a dependency to update:")

        # Add buttons for each dependency found in the manifest
        for dep in dependencies:
            form.button(dep.get("module_name") + " - " + dep.get("version"))

        # Add Cancel Button
        form.button("Cancel")

        # Show the form to the player and proceed based on their selection
        form.show(player).then(
            lambda p=player, result=ActionFormResponse: select_version(p, result, dependencies, manifest_path, chosen_pack, path)
        )

    except Exception as e:
        player.send_message(f"Failed to load dependencies for {chosen_pack}: {e}")

def select_version(player: Player, result: ActionFormResponse, dep, manifest_path, chosen_pack, path):
    if result.canceled or int(result.selection) >= len(dep):
        return

    # Get the selected dependency
    chosen_dep = dep[result.selection] if result.selection is not None else None
    if not chosen_dep:
        player.send_message(f"Action canceled or invalid selection.")
        return

    module_name = chosen_dep.get("module_name")
    npm_url = f"https://registry.npmjs.org/{module_name}"

    try:
        response = requests.get(npm_url)
        response.raise_for_status()

        data = response.json()
        all_versions = list(data.get("versions", {}).keys())[::-1]  # Reverse to show newest first

    except requests.RequestException as e:
        player.send_message(f"Error fetching versions for {module_name}: {e}")
        return

    def show_version_menu(page=0):

        page_lim = 10

        form = ActionFormData()
        total_pages = (len(all_versions) // page_lim) + 1
        form.title(f"{module_name} Ver (Page {page + 1}/{total_pages})")
        form.body("Choose a version:")

        start_index = page * page_lim
        end_index = min(start_index + page_lim, len(all_versions))
        page_versions = all_versions[start_index:end_index]

        for version in page_versions:
            form.button(version)

        if start_index > 0:
            form.button("<- Previous Page")
        if end_index < len(all_versions):
            form.button("Next Page ->")

        def on_version_selected(player: Player, response: ActionFormResponse):
            if response.canceled:
                return

            index = response.selection
            total_buttons = len(page_versions)

            if start_index > 0 and index == total_buttons:  # Previous Page
                show_version_menu(page - 1)
            elif end_index < len(all_versions) and index == total_buttons + (start_index > 0):  # Next Page
                show_version_menu(page + 1)
            else:  # A version was selected
                chosen_version = page_versions[int(index)]

                lower_version = chosen_version.lower()
                if "beta" in lower_version or "preview" in lower_version:
                    split_version = chosen_version.split(".")
                    for i in range(len(split_version)):
                        if "beta" in split_version[i].lower() or "preview" in split_version[i].lower():
                            major_version = ".".join(split_version[:i + 1])  # Keep everything up to "beta" or "preview"
                            break
                    else:
                        major_version = chosen_version  # Fallback
                else:
                    major_version = chosen_version  # Keep full version

                try:
                    manifest_content = open_text_file(manifest_path, mode="r")
                    manifest = json.loads(manifest_content)

                    # Loop through the list of dependencies to find the correct one to update
                    for dependency in manifest["dependencies"]:
                        if dependency["module_name"] == module_name:  # Assuming 'module_name' is provided as input
                            dependency["version"] = major_version  # Update the version
                            break  # Stop once the correct dependency is updated

                    # Save the updated manifest back to the file
                    manifest_text = json.dumps(manifest, indent=4)
                    result = open_text_file(manifest_path, mode="w", text=manifest_text)
                    if result is None:
                        print(f"[PrimeBDS] Failed to write manifest to {manifest_path}")

                    select_pack(player, path)

                except Exception as e:
                    print(f"Error reading or writing manifest file: {e}")

                player.send_message(f"Selected {module_name} - {chosen_version} §7(§fSaved as §e{major_version}§7)\nUpdated {chosen_pack} §7- §oREQUIRES SERVER RESTART TO APPLY")

        form.show(player).then(
            lambda p=player, result=ActionFormResponse: on_version_selected(p, result)
        )

    show_version_menu()
