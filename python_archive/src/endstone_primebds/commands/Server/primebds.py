import re
from endstone import Player
from endstone.command import CommandSender

from endstone_primebds.utils.command_util import create_command
from endstone_primebds.utils.config_util import load_config, save_config, reload_config, save_cmd_config, load_cmd_config
from endstone_primebds.utils.form_wrapper_util import ModalFormData, ModalFormResponse, ActionFormData, ActionFormResponse

from typing import TYPE_CHECKING
if TYPE_CHECKING:
    from endstone_primebds.primebds import PrimeBDS

command, permission = create_command(
    "primebds",
    "An all-in-one primebds manager!",
    ["/primebds (config|command|info|reloadconfig)[primebds_subaction: primebds_subaction]"],
    ["primebds.command.primebds"]
)

# PRIMEBDS FUNCTIONALITY
def handler(self: "PrimeBDS", sender: CommandSender, args: list[str]) -> bool:
    if len(args) == 0:
        sender.send_message(f"§dPrimeBDS\n§d{self.description}\n\n§dIf this plugin has helped you at all, consider leaving a star:\n§e@ https://github.com/PrimeStrat/primebds\n\n§dConfused on how something works?\nVisit the wiki:\n§e@ https://github.com/PrimeStrat/primebds/wiki")
        return True

    if args[0].lower() == "config" and sender.is_op:
        if not isinstance(sender, Player):
            sender.send_error_message("§cThis command can only be executed by a player")
            return True
        open_config_categories(sender)
    elif args[0].lower() == "config" and not sender.is_op:
        if not isinstance(sender, Player):
            sender.send_error_message("§cThis command can only be executed by a player")
            return True
        sender.send_error_message("§cOnly operators can modify the server config")
    elif args[0].lower() == "command":
        if not isinstance(sender, Player):
            sender.send_error_message("§cThis command can only be executed by a player")
            return True
        command_form(self, sender)
    elif args[0].lower() == "reloadconfig":
        reload_config()
        sender.send_message(f"§dPrimeBDS config reloaded!")
    elif args[0].lower() == "info":
        sender.send_message(f"§dPrimeBDS\n§d{self.description}\n\n§dIf this plugin has helped you at all, consider leaving a star:\n§e@ https://github.com/PrimeStrat/primebds\n\n§dConfused on how something works?\nVisit the wiki:\n§e@ https://github.com/PrimeStrat/primebds/wiki")

    return True

def command_form(self: "PrimeBDS", sender: Player):
    form = ActionFormData()
    form.title("Command GUI")
    form.body("Choose a command to run:")

    cmds = [self.get_command(c) for c in self.commands if self.get_command(c)]
    filtered_cmds = [c for c in cmds if not c.permissions or sender.has_permission(c.permissions[0])]
    filtered_cmds.sort(key=lambda c: c.name.lower())

    for cmd in filtered_cmds:
        form.button(f"§4/{cmd.name}")

    def submit(player: Player, result: ActionFormResponse):
        if result.canceled or result.selection >= len(filtered_cmds):
            return
        cmd = filtered_cmds[result.selection]
        open_usage_selector(self, player, cmd)

    form.show(sender).then(lambda player=sender, result=None: submit(player, result))

def open_usage_selector(self: "PrimeBDS", player: Player, cmd):
    usages = cmd.usages

    enum_map = {}
    non_enum_usages = []

    for usage in usages:
        parsed_args = parse_usage(usage)
        if not parsed_args:
            continue
        first_arg = parsed_args[0]
        name, arg_type, required, enum_values = first_arg

        if enum_values:
            for ev in enum_values:
                enum_map.setdefault(ev, []).append(usage)
        else:
            non_enum_usages.append(usage)

    if enum_map:
        form = ActionFormData()
        form.title(f"/{cmd.name} Subcommand")
        form.body("Choose an option:")

        enum_keys = list(enum_map.keys())
        for ev in enum_keys:
            form.button(ev)

        def submit_enum(player: Player, result: ActionFormResponse):
            if result.canceled:
                return
            choice = enum_keys[result.selection]
            selected_usage = enum_map[choice][0]
            parsed_args = parse_usage(selected_usage)
            remaining_args = parsed_args[1:]
            open_argument_form(self, player, cmd, remaining_args, prefilled=[choice])

        form.show(player).then(lambda player=player, result=None: submit_enum(player, result))
    else:
        open_argument_form(self, player, cmd, parse_usage(non_enum_usages[0]) if non_enum_usages else [])

def open_argument_form(self: "PrimeBDS", player: Player, cmd, all_args, prefilled=None):
    if prefilled is None:
        prefilled = []

    form = ModalFormData()
    form.title(f"/{cmd.name}")

    for (name, arg_type, required, enum_values) in all_args:
        add_argument_field(form, name, arg_type, enum_values)

    def submit_args(p: Player, result: ModalFormResponse):
        if result.canceled:
            return

        args = prefilled.copy()
        for i, (name, arg_type, required, enum_values) in enumerate(all_args):
            value = result.formValues[i]

            if enum_values:
                args.append(enum_values[value])
            elif arg_type == "bool":
                args.append("true" if value else "false")
            else:
                args.append(str(value).strip())

        arg_string = " ".join(args)
        p.perform_command(f"{cmd.name} {arg_string}")

    form.show(player).then(lambda p=player, result=None: submit_args(p, result))

def add_argument_field(form, name: str, arg_type: str, enum_values: list[str] = None):
    if enum_values:
        form.dropdown(f"{name} (enum)", enum_values, 0)
    elif arg_type == "int":
        form.text_field(f"{name} (integer)", "10")
    elif arg_type == "float":
        form.text_field(f"{name} (float)", "3.14")
    elif arg_type == "bool":
        form.toggle(f"{name} (true/false)", False)
    elif arg_type in ("target", "actor", "entity", "player"):
        form.text_field(f"{name} (target selector)", "@a, @p, PlayerName")
    elif arg_type == "str":
        form.text_field(f"{name} (string)", "Hello")
    elif arg_type == "block_pos":
        form.text_field(f"{name} (x y z)", "1 2 3")
    elif arg_type == "pos":
        form.text_field(f"{name} (x y z float)", "1.0 2.0 3.0")
    elif arg_type == "message":
        form.text_field(f"{name} (message)", "Hello World!")
    elif arg_type == "json":
        form.text_field(f"{name} (json)", '{"key":"value"}')
    elif arg_type == "block":
        form.text_field(f"{name} (block id)", "stone")
    elif arg_type == "block_states":
        form.text_field(f"{name} (block states)", '["wood_type"="birch","stripped_bit"=true]')
    else:
        form.text_field(f"{name} (string)", name)

    return form

def parse_usage(usage: str):
    args = []
    token_pattern = r"(\([^)]+\))?\s*(<([^>]+)>|\[([^\]]+)\])"
    for enum_part, _, required_part, optional_part in re.findall(token_pattern, usage):
        token = required_part or optional_part
        is_required = bool(required_part)

        parts = token.split(":")
        if len(parts) == 2:
            name, arg_type = parts[0].strip(), parts[1].strip()
        else:
            name, arg_type = token.strip(), "str"

        enum_values = None
        if enum_part:
            enum_values = enum_part.strip("()").split("|")  

        args.append((name, arg_type, is_required, enum_values))

    return args

def open_config_categories(player: Player):
    form = ActionFormData()
    form.title("PrimeBDS Config")
    form.body("Select a category to edit:")

    base_config = load_config()
    categories = list(base_config.keys())
    categories.append("commands")

    for category in categories:
        form.button(f"§4{category.capitalize()}")

    form.button("Close")

    def submit(player: Player, result: ActionFormResponse):
        if result.canceled or result.selection is None:
            return

        if result.selection == len(categories):
            return

        category = categories[result.selection]
        if category == "commands":
            selected_config = load_cmd_config()
        else:
            selected_config = load_config()

        open_category_editor(player, category, selected_config)

    form.show(player).then(lambda player=player, result=ActionFormResponse: submit(player, result))

def open_category_editor(player: Player, category: str, config: dict):
    if category not in config:
        player.send_message(f"§cCategory '{category}' not found.")
        return

    settings = config[category]

    if category.lower() == "commands":
        form = ModalFormData()
        form.title("Commands Configuration")
        field_map = []

        for cmd, value_dict in settings.items():
            enabled = value_dict.get("enabled", False)
            field_map.append((cmd, bool))
            form.toggle(f"/{cmd}", enabled)

        form.submit_button("Save Changes")

        def submit(player: Player, response: ModalFormResponse):
            if response.canceled:
                open_config_categories(player)
                return

            new_values = response.formValues
            updated = {}

            for i, (cmd, _) in enumerate(field_map):
                new_enabled = bool(new_values[i])
                settings[cmd]["enabled"] = new_enabled
                updated[cmd] = new_enabled

            config[category] = settings
            save_cmd_config(config, True)

            player.send_message("§aCommands updated!")
            player.send_message("§aRun §e/reload §ato apply changes!")

        form.show(player).then(lambda player=player, response=ModalFormResponse: submit(player, response))
        return

    else:
        if any(isinstance(v, dict) for v in settings.values()):
            open_module_editor(player, category, settings, config)
            return

def open_module_editor(player: Player, module_name: str, settings: dict, config: dict, parent_name: str = None):
    subkeys = [k for k, v in settings.items() if isinstance(v, dict)]
    primitive_keys = [k for k, v in settings.items() if not isinstance(v, dict)]

    def primitives():
        if primitive_keys:
            form = ModalFormData()
            form.title(f"{format_label(module_name)} Configuration")
            field_map = []

            for key in primitive_keys:
                value = settings[key]
                value_type = type(value)
                field_map.append((key, value_type))

                if isinstance(value, bool):
                    form.toggle(format_label(key), value)
                else:
                    form.text_field(format_label(key), str(value), str(value))

            form.submit_button("Save Changes")

            def submit_modal(player: Player, response: ModalFormResponse):
                if response.canceled:
                    open_config_categories(player)
                    return

                new_values = response.formValues
                updated = {}

                for i, (key, value_type) in enumerate(field_map):
                    old_value = settings[key]
                    new_value = new_values[i]

                    if value_type == bool:
                        new_value = bool(new_value)
                    if value_type == int:
                        try:
                            new_value = float(new_value)
                        except ValueError:
                            new_value = old_value
                    elif value_type == float:
                        try:
                            new_value = float(new_value)
                        except ValueError:
                            new_value = old_value
                    elif value_type is list:
                        raw = str(new_values[i]).strip()

                        if raw.startswith("[") and raw.endswith("]"):
                            raw = raw[1:-1].strip()

                        parts = raw.split(",")

                        new_value = [
                            p.strip().strip("'").strip('"')
                            for p in parts
                            if p.strip()
                        ]

                    else:
                        if not isinstance(new_value, bool):
                            new_value = str(new_value)

                    settings[key] = new_value
                    updated[key] = new_value

                save_config(config, True)

                if updated:
                    player.send_message(f"§aUpdated values for {format_label(module_name)}")
                    if module_name == "permissions_manager" or module_name == "afk":
                        player.send_message(f"§cThis module requires a §e/reload §cto apply changes")

                if subkeys:
                    open_module_editor(player, module_name, settings, config, parent_name)

            form.show(player).then(lambda player=player, response=ModalFormResponse: submit_modal(player, response))
            return

        if subkeys:
            form = ActionFormData()
            form.title(f"{format_label(module_name)} Subcategories")
            form.body("Select a subcategory to edit:")

            for sub in subkeys:
                form.button(f"§4{format_label(sub)}")

            form.button("Back")
            form.button("Close")

            def submit_action(player: Player, result: ActionFormResponse):
                if result.canceled:
                    open_config_categories(player)
                    return

                selected_index = result.selection
                if selected_index is None:
                    open_config_categories(player)
                    return

                if selected_index == len(subkeys):
                    if parent_name:
                        open_module_editor(player, parent_name, config[parent_name], config)
                    else:
                        open_config_categories(player)
                    return

                if selected_index == len(subkeys) + 1:
                    return

                chosen_key = subkeys[selected_index]
                open_module_editor(player, chosen_key, settings[chosen_key], config, module_name)

            form.show(player).then(lambda player=player, result=ActionFormResponse: submit_action(player, result))

    if module_name.lower() == "combat":
        form = ActionFormData()
        form.title("Combat Configuration")
        form.body("Select what to edit:")

        form.button("Normal Settings")
        form.button("Projectiles")
        form.button("Tag Overrides")
        form.button("Back")

        def submit_combat(player: Player, result: ActionFormResponse):
            if result.canceled or result.selection is None:
                open_config_categories(player)
                return

            if result.selection == 0:
                primitives()
                return
            elif result.selection == 1:
                open_projectiles(player)
                return
            elif result.selection == 2:
                open_tag_overrides(player)
                return
            elif result.selection == 3:
                if parent_name:
                    open_module_editor(player, parent_name, config[parent_name], config)
                else:
                    open_config_categories(player)

        form.show(player).then(lambda player=player, result=ActionFormResponse: submit_combat(player, result))
        return

    primitives()
    
def open_projectiles(player: Player):
    config = load_config()
    proj = config["modules"]["combat"]["projectiles"]

    form = ModalFormData()
    form.title("Projectile Settings")

    form.text_field("Horizontal KB Modifier", "e.g. 0.0", str(proj["horizontal_knockback_modifier"]))
    form.text_field("Vertical KB Modifier", "e.g. 0.0", str(proj["vertical_knockback_modifier"]))

    def submit(player: Player, result: ModalFormResponse):
        if result.canceled:
            return
        try:
            config["modules"]["combat"]["projectiles"]["horizontal_knockback_modifier"] = float(result.formValues[0])
            config["modules"]["combat"]["projectiles"]["vertical_knockback_modifier"] = float(result.formValues[1])
            save_config(config, True)
            player.send_message("§aProjectile settings updated!")
        except ValueError:
            player.send_message("§cInvalid input, please enter numbers.")

    form.show(player).then(lambda player=player, result=None: submit(player, result))

def open_tag_overrides(player: Player):
    config = load_config()
    tag_overrides = config["modules"]["combat"]["tag_overrides"]

    form = ActionFormData()
    form.title("Tag Overrides")
    form.body("Select a tag to edit:")

    tags = list(tag_overrides.keys())
    if not tags:
        player.send_message("§cNo tag overrides defined.")
        return

    for tag in tags:
        form.button(tag)

    def submit(player: Player, result: ActionFormResponse):
        if result.canceled or result.selection >= len(tags):
            return
        chosen_tag = tags[result.selection]
        edit_tag_override(player, chosen_tag)

    form.show(player).then(lambda player=player, result=None: submit(player, result))

def edit_tag_override(player: Player, tag: str):
    config = load_config()
    settings = config["modules"]["combat"]["tag_overrides"][tag]
    projectiles = settings.get("projectiles", {})

    form = ModalFormData()
    form.title(f"Edit Tag: {tag}")

    for key, value in settings.items():
        if key == "projectiles":
            continue

        if isinstance(value, bool):
            form.toggle(format_label(key), value)
        else:
            form.text_field(format_label(key), str(value), str(value))

    for key, value in projectiles.items():
        label = f"Projectile {format_label(key)}"
        if isinstance(value, bool):
            form.toggle(label, value)
        else:
            form.text_field(label, str(value), str(value))


    def submit(player: Player, result: ModalFormResponse):
        if result.canceled:
            return

        values = result.formValues
        idx = 0

        for key in settings:
            if key == "projectiles":
                continue
            value = settings[key]
            if isinstance(value, bool):
                settings[key] = values[idx]
            elif isinstance(value, int):
                settings[key] = float(values[idx]) if values[idx] else 0
            elif isinstance(value, float):
                settings[key] = float(values[idx]) if values[idx] else 0.0
            else:
                settings[key] = values[idx]
            idx += 1

        for key in projectiles:
            value = projectiles[key]
            if isinstance(value, bool):
                projectiles[key] = values[idx]
            elif isinstance(value, int):
                projectiles[key] = float(values[idx]) if values[idx] else 0
            elif isinstance(value, float):
                projectiles[key] = float(values[idx]) if values[idx] else 0.0
            else:
                projectiles[key] = values[idx]
            idx += 1

        settings["projectiles"] = projectiles
        save_config(config, True)
        player.send_message(f"§aTag override '{tag}' updated!")

    form.show(player).then(lambda player=player, result=None: submit(player, result))

def format_label(key: str) -> str:
    return " ".join(word.capitalize() for word in key.split("_"))
