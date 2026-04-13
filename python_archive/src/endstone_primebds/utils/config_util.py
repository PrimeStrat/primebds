import json
import os
import copy

current_dir = os.path.dirname(os.path.abspath(__file__))
while not (os.path.exists(os.path.join(current_dir, 'plugins')) and os.path.exists(os.path.join(current_dir, 'worlds'))):
    current_dir = os.path.dirname(current_dir)

CONFIG_FOLDER = os.path.join(current_dir, 'plugins', 'primebds_data')
CONFIG_PATH = os.path.join(CONFIG_FOLDER, 'config.json')
CMD_CONFIG_PATH = os.path.join(CONFIG_FOLDER, 'commands.json')
PERMISSIONS_PATH = os.path.join(CONFIG_FOLDER, 'permissions.json')
RULES_PATH = os.path.join(CONFIG_FOLDER, 'rules.txt')

os.makedirs(CONFIG_FOLDER, exist_ok=True)

PERMISSIONS_DEFAULT = {
    "Default": {
        "permissions": {
            "endstone.broadcast": True,
            "endstone.broadcast.user": True,
            "endstone.command.version": True,
            "endstone.command.plugins": True,
            "primebds.command.ping": True,
            "primebds.command.reply": True,
            "minecraft.command.list": True,
            "minecraft.command.tell": True,
            "minecraft.command.me": True,
        },
        "inherits": [],
        "weight": 0
    },
    "Operator": {
        "permissions": {
            "*": True
        },
        "inherits": ["Default"],
        "weight": 100,
        "prefix": "§8[§cAdmin§8] §c",
        "suffix": "§r"
    },
}

cache = None 
cmd_cache = None
permissions_cache = None
rules_cache = None

def load_cmd_config():
    """Load or create a configuration file in primebds_info/commands.json, cached in memory."""
    global cmd_cache
    if cmd_cache is not None:
        return cmd_cache

    default_cmd_config = {}

    if not os.path.exists(CMD_CONFIG_PATH):
        os.makedirs(os.path.dirname(CMD_CONFIG_PATH), exist_ok=True)
        cmd_cache = default_cmd_config
        save_cmd_config(cmd_cache)
        return cmd_cache

    try:
        content = open_text_file(CMD_CONFIG_PATH, "r")
        if content:
            cmd_cache = json.loads(content)
        else:
            cmd_cache = default_cmd_config
            save_cmd_config(cmd_cache)
    except (OSError, json.JSONDecodeError):
        cmd_cache = default_cmd_config
        save_cmd_config(cmd_cache)

    return cmd_cache

def save_cmd_config(config: dict, update_cache: bool = False) -> None:
    """Save to commands.json."""
    global cmd_cache
    if update_cache:
        cmd_cache = config

    text = json.dumps(config, indent=4)
    open_text_file(CMD_CONFIG_PATH, "w", text=text)

def load_config():
    """Load config.json safely, return defaults if missing, but don't overwrite immediately."""
    global cache
    if cache is not None:
        return cache

    default_config = {"commands": {}}

    if not os.path.exists(CONFIG_PATH):
        os.makedirs(os.path.dirname(CONFIG_PATH), exist_ok=True)
        cache = default_config
        # DON'T save immediately
        print(f"[PrimeBDS] Config file not found, using defaults in memory.")
        return cache

    try:
        with open(CONFIG_PATH, "r", encoding="utf-8") as f:
            content = f.read()
        if content.strip():  # non-empty file
            cache = json.loads(content)
        else:
            cache = default_config
            # DON'T save immediately
            print(f"[PrimeBDS] Config file empty, using defaults in memory.")
    except json.JSONDecodeError as e:
        print(f"[PrimeBDS] JSON error in config.json: {e}. Using defaults in memory.")
        cache = default_config
    except OSError as e:
        print(f"[PrimeBDS] Failed to read config.json: {e}. Using defaults in memory.")
        cache = default_config

    return cache

def reload_config():
    """Reload all configuration caches."""
    global cache, cmd_cache, permissions_cache, rules_cache
    cache = None
    cmd_cache = None
    permissions_cache = None
    rules_cache = None

    load_config()
    load_cmd_config()
    load_rules()
    load_permissions()

def save_config(config: dict, update_cache: bool = False) -> None:
    global cache
    if update_cache:
        cache = config

    text = json.dumps(config, indent=4)
    open_text_file(CONFIG_PATH, "w", text=text)

def save_permissions(permissions: dict, update_cache: bool = True) -> None:
    global permissions_cache

    os.makedirs(CONFIG_FOLDER, exist_ok=True)
    content = json.dumps(permissions, indent=4)
    open_text_file(PERMISSIONS_PATH, mode="w", text=content)

    if update_cache:
        permissions_cache = copy.deepcopy(permissions)

def load_permissions(default_permissions=None, cache=True):
    global permissions_cache
    if permissions_cache is not None and cache:
        return permissions_cache

    if not os.path.exists(PERMISSIONS_PATH):
        if default_permissions is None:
            default_permissions = PERMISSIONS_DEFAULT
        save_permissions(default_permissions)
        permissions_cache = copy.deepcopy(default_permissions)
    else:
        content = open_text_file(PERMISSIONS_PATH, mode="r")
        try:
            permissions_cache = json.loads(content) if content else default_permissions or PERMISSIONS_DEFAULT
        except (json.JSONDecodeError, TypeError):
            permissions_cache = default_permissions or PERMISSIONS_DEFAULT
            save_permissions(permissions_cache)

    updated = False
    for key in ["Default", "Operator"]:
        if key not in permissions_cache:
            permissions_cache[key] = PERMISSIONS_DEFAULT.get(key, {})
            updated = True

    if updated:
        save_permissions(permissions_cache)
        
    return permissions_cache

def reset_permissions(default_permissions=None):
    global permissions_cache

    if default_permissions is None:
        default_permissions = PERMISSIONS_DEFAULT

    open_text_file(PERMISSIONS_PATH, "w", text=json.dumps(default_permissions, indent=4))
    permissions_cache = copy.deepcopy(default_permissions)

def save_rules(rules):
    os.makedirs(CONFIG_FOLDER, exist_ok=True)
    content = "\n".join(line.strip() for line in rules) + "\n"
    open_text_file(RULES_PATH, mode="w", text=content)

def load_rules(default_rules=None):
    global rules_cache
    if rules_cache:
        return rules_cache

    if not os.path.exists(RULES_PATH):
        if default_rules is None:
            default_rules = []
        save_rules(default_rules)
        rules_cache = default_rules
    else:
        content = open_text_file(RULES_PATH, "r")
        if content is None:
            return []
        return [line.strip() for line in content.splitlines() if line.strip()]
    return rules_cache

def save_properties_file(path: str, props: dict) -> bool:
    try:
        lines = []
        for key, value in props.items():
            if isinstance(value, bool):
                value_str = "true" if value else "false"
            else:
                value_str = str(value)
            lines.append(f"{key}={value_str}")

        content = "\n".join(lines) + "\n"

        result = open_text_file(path, mode="w", text=content)
        return result is not None

    except Exception as e:
        print(f"Failed to save properties file {path}: {e}")
        return False

def find_and_load_config(
    filename,
    start_path=None,
    forbidden_subdir="multiworld",
    max_depth=20,
    refresh=False
):
    if not hasattr(find_and_load_config, "_cache"):
        find_and_load_config._cache = {}

    if not refresh and filename in find_and_load_config._cache:
        return find_and_load_config._cache[filename]

    if start_path is None:
        start_path = os.getcwd()

    current_path = os.path.abspath(start_path)
    depth = 0

    while depth < max_depth:
        candidate_path = os.path.join(current_path, filename)

        if os.path.isfile(candidate_path):
            norm_path = os.path.normpath(candidate_path)
            path_parts = norm_path.split(os.sep)
            if forbidden_subdir not in path_parts:
                try:
                    if filename.endswith(".json"):
                        content = open_text_file(candidate_path)
                        config_data = json.loads(content)
                    elif filename.endswith(".properties"):
                        config_data = parse_properties_file(candidate_path)
                    else:
                        raise ValueError(f"Unsupported file type: {filename}")

                    find_and_load_config._cache[filename] = config_data
                    return config_data
                except Exception as e:
                    print(f"Failed to load {filename}: {e}")
                    if refresh:
                        # if forcing reload, don't return stale cache
                        return None
                    return find_and_load_config._cache.get(filename)

        parent = os.path.dirname(current_path)
        if parent == current_path:
            break
        current_path = parent
        depth += 1

    find_and_load_config._cache[filename] = None
    return None

def find_folder(
    foldername,
    start_path=None,
    forbidden_subdir="multiworld",
    max_depth=20,
    refresh=False
):
    if not hasattr(find_folder, "_cache"):
        find_folder._cache = {}

    if not refresh and foldername in find_folder._cache:
        return find_folder._cache[foldername]

    if start_path is None:
        start_path = os.getcwd()

    current_path = os.path.abspath(start_path)
    depth = 0

    while depth < max_depth:
        candidate_path = os.path.join(current_path, foldername)

        if os.path.isdir(candidate_path):
            norm_path = os.path.normpath(candidate_path)
            path_parts = norm_path.split(os.sep)
            if forbidden_subdir not in path_parts:
                find_folder._cache[foldername] = candidate_path
                return candidate_path

        parent = os.path.dirname(current_path)
        if parent == current_path:
            break
        current_path = parent
        depth += 1

    find_folder._cache[foldername] = None
    return None

def find_server_properties(start_path: str = None) -> str | None:
    if start_path is None:
        start_path = os.getcwd()
    start_path = os.path.abspath(start_path)

    current_dir = start_path
    while True:
        candidate = os.path.join(current_dir, "server.properties")
        if os.path.isfile(candidate):
            return candidate

        parent_dir = os.path.dirname(current_dir)
        if parent_dir == current_dir:  # Reached root without finding file
            break
        current_dir = parent_dir

    return None

def parse_properties_file(path: str) -> dict:
    props = {}
    content = open_text_file(path)

    for line in content.splitlines():
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        if "=" in line:
            key, val = line.split("=", 1)
            props[key.strip()] = val.strip()
    return props

def open_text_file(path: str, mode: str = "r", text: str = None) -> str | None: # hopefully fixes multiworld issues
    """
    Universal text file handler.
    - For reading: mode="r", returns file content as str.
    - For writing: mode="w", text=<string to write>
    Tries UTF-8, UTF-8-sig, Latin-1, CP1252 automatically.
    """
    encodings = ["utf-8", "utf-8-sig", "latin-1", "cp1252"]

    if "r" in mode:
        for enc in encodings:
            try:
                with open(path, mode, encoding=enc) as f:
                    return f.read()
            except (UnicodeDecodeError, FileNotFoundError):
                continue
        return None

    elif "w" in mode and text is not None:
        for enc in encodings:
            try:
                with open(path, mode, encoding=enc) as f:
                    f.write(text)
                    return text
            except UnicodeEncodeError:
                continue
        return None

    else:
        raise ValueError("Invalid mode or missing text for writing.")