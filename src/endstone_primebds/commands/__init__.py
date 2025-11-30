import importlib
import json
import pkgutil
import os

import endstone_primebds

from endstone_primebds.utils.config_util import load_config, save_config, load_permissions, load_rules, load_cmd_config, save_cmd_config, PERMISSIONS_DEFAULT
from collections import defaultdict, OrderedDict

# Global storage for preloaded commands
preloaded_commands = {}
preloaded_permissions = {}
preloaded_handlers = {}

from collections import OrderedDict
import os
import json

def preload_settings():
    """Preload all plugin settings with defaults if missing, preserving key order and example structures,
    but do NOT overwrite the config file immediately if something is wrong."""
    
    config = load_config()  # safe loading
    
    default_modules = OrderedDict({
        "afk": OrderedDict({
            "broadcast_afk_status": True,
            "constantly_check_afk_status": False,
            "idle_threshold": 300
        }),
        "back": OrderedDict({
            "save_unnatural_teleports": True,
            "save_death_locations": True
        }),
        "broadcast": OrderedDict({
            "prefix": "§l§8[§c!§8] §r§e",
            "playsound": "random.toast"
        }),
        "rtp": OrderedDict({
            "x": 0,
            "z": 0,
            "min_distance": 100,
            "radius": 1000,
            "cost": 0,
            "delay": 0,
            "cooldown": 0
        }),
        "permissions_manager": OrderedDict({
            "primebds": True,
            "endstone": True,
            "minecraft": True,
            "*": True
        }),
        "discord": OrderedDict({
            "command": "§cUnset"
        }),
        "discord_webhook": OrderedDict({
            "embed_for_log": OrderedDict({
                "enabled": False,
                "color": 781919,
                "title": "Logger"
            }),
            "command_logs": OrderedDict({
                "enabled": False,
                "webhook": ""
            }),
            "moderation_logs": OrderedDict({
                "enabled": False,
                "webhook": ""
            }),
            "chat_logs": OrderedDict({
                "enabled": False,
                "webhook": ""
            }),
            "connection_logs": OrderedDict({
                "enabled": False,
                "webhook": ""
            })
        }),
        "spectator_check": OrderedDict({
            "check_gamemode": True,
            "force_gamemode": True,
            "check_tags": False,
            "allow_tags": [],
            "ignore_tags": []
        }),
        "me_crasher_patch": OrderedDict({
            "enabled": True,
            "ban": False
        }),
        "join_leave_messages": OrderedDict({
            "send_on_nick": False,
            "send_on_vanish": False,
            "send_on_connection": False,
            "join_message": "§e{player} joined the game",
            "leave_message": "§e{player} left the game",
            "shutdown": "Server has shutdown!"
        }),
        "server_optimizer": OrderedDict({
            "mute_laggy_sounds": True,
            "mute_laggy_block_events": True,
            "mute_laggy_movement_updates": False
        }),
        "server_messages": OrderedDict({
            "skin_change_messages": True,
            "enhanced_whispers": True,
            "enhanced_chat": True,
            "rank_meta_data": True,
            "rank_meta_nametags": False,
            "chat_cooldown": 0,
            "chat_prefix": "§r: ",
            "whisper_prefix": "§8[§bWhisper§8]§r ",
            "social_spy_prefix": "§8[§bSocial Spy§8]§r ",
            "staff_chat_prefix": "§8[§bStaff Chat§8]§r "
        }),
        "message_of_the_day": OrderedDict({
            "message_of_the_day_command": "§cUnset",
            "send_message_of_the_day_on_connect": False
        }),
        "combat": OrderedDict({
            "hit_cooldown_in_seconds": 0.0,
            "base_damage": 1.0,
            "horizontal_knockback_modifier": 0.0,
            "vertical_knockback_modifier": 0.0,
            "horizontal_sprint_knockback_modifier": 0.0,
            "vertical_sprint_knockback_modifier": 0.0,
            "fall_damage_height": 3.5,
            "projectiles": OrderedDict({
                "horizontal_knockback_modifier": 0.0,
                "vertical_knockback_modifier": 0.0
            }),
            "disable_fire_damage": False,
            "disable_explosion_damage": False,
            "disable_sprint_hits": False,
            "tag_overrides": OrderedDict({
                "example_tag": OrderedDict({
                    "hit_cooldown_in_seconds": 1,
                    "base_damage": 5,
                    "horizontal_knockback_modifier": 2,
                    "vertical_knockback_modifier": 2,
                    "horizontal_sprint_knockback_modifier": 0,
                    "vertical_sprint_knockback_modifier": 0,
                    "fall_damage_height": 7.0,
                    "projectiles": OrderedDict({
                        "horizontal_knockback_modifier": 0.0,
                        "vertical_knockback_modifier": 0.0
                    }),
                    "disable_fire_damage": True,
                    "disable_explosion_damage": True,
                    "disable_sprint_hits": True
                })
            })
        }),
        "multiworld": OrderedDict({
            "worlds": OrderedDict({
                "example": OrderedDict({
                    "enabled": False,
                    "server-port": 19134,
                    "server-portv6": 19135,
                    "level-name": "example",
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
                    "allow-list": True,
                    "allow-cheats": True
                })
            })
        })
    })

    # Ensure the modules section exists
    config.setdefault("modules", OrderedDict())

    # Only add missing top-level modules, preserving existing ones completely
    changed = False
    for module, defaults in default_modules.items():
        if module not in config["modules"]:
            config["modules"][module] = defaults
            changed = True

    # Remove unknown top-level modules (optional; can be commented if you want full safety)
    # for module in list(config["modules"].keys()):
    #     if module not in default_modules:
    #         del config["modules"][module]
    #         changed = True

    # Save only if new modules were added, never overwrite existing ones unnecessarily
    if changed:
        try:
            save_config(config)
        except Exception as e:
            print(f"Failed to save config.json: {e}. Existing file left untouched.")

def preload_commands():
    """Preload all command modules before PrimeBDS is instantiated, respecting the config."""
    global preloaded_commands, preloaded_permissions, preloaded_handlers

    commands_base_path = os.path.join(os.path.dirname(endstone_primebds.__file__), 'commands')
    config = load_cmd_config()

    config.setdefault("commands", {})

    grouped_commands = defaultdict(list)
    found_commands = set()

    print("[PrimeBDS] Registering commands...")

    for root, _, _ in os.walk(commands_base_path):
        rel_path = os.path.relpath(root, commands_base_path)
        package_path = rel_path.replace(os.sep, ".") if rel_path != "." else ""

        for _, module_name, _ in pkgutil.iter_modules([root]):
            module_import_path = f"endstone_primebds.commands{('.' + package_path) if package_path else ''}.{module_name}"
            module = importlib.import_module(module_import_path)

            if hasattr(module, 'command') and hasattr(module, 'handler'):
                for cmd, details in module.command.items():
                    found_commands.add(cmd) 

                    if cmd not in config["commands"]:
                        config["commands"][cmd] = {"enabled": True}

                    if config["commands"][cmd]["enabled"]:
                        preloaded_commands[cmd] = details
                        preloaded_handlers[cmd] = module.handler

                        grouped_commands[package_path].append((cmd, details.get('description', 'No description')))
                    else:
                        grouped_commands[package_path].append((cmd, "Disabled by config"))

                if hasattr(module, 'permission'):
                    for perm, details in module.permission.items():
                        preloaded_permissions[perm] = details

    # Remove commands that are no longer found
    removed_commands = set(config["commands"].keys()) - found_commands
    for cmd in removed_commands:
        del config["commands"][cmd]

    # Print grouped commands
    for category, commands in grouped_commands.items():
        if category or commands:  # Only print "Root" if it has commands
            clean_category = category.replace("_", " ") if category else "Root"
            print(f"\n[{clean_category}]")
            for cmd, desc in commands:
                status = "✓" if "Disabled by config" not in desc else "✗"
                print(f"{status} {cmd} - {desc}")

    # Print removed commands
    if removed_commands:
        print("\n[PrimeBDS] Removed missing commands:")
        for cmd in removed_commands:
            print(f"✗ {cmd}")

    print("\n")
    save_cmd_config(config)

def preload_permissions():
    """Preload permissions.json with defaults if missing, preserving key order and removing unknown keys."""
    load_permissions(PERMISSIONS_DEFAULT, False)

# Run preload automatically when this file is imported
preload_permissions()
preload_settings()
preload_commands()
print(f"\n[PrimeBDS] Loaded {len(preloaded_commands)} commands")

# ADDITIONAL DEFAULTS
RULES_DEFAULT = [
    "§6Rules:",
    "§7-------------------------",
    "§b1. §fBe respectful to others.",
    "§b2. §fNo griefing or stealing.",
    "§b3. §fNo cheating or exploiting bugs.",
]

load_rules(RULES_DEFAULT)

__all__ = [preloaded_commands, preloaded_permissions, preloaded_handlers]
