import time

from endstone import Player
from endstone_primebds.utils.config_util import load_permissions, load_config

from typing import TYPE_CHECKING
if TYPE_CHECKING:
    from endstone_primebds.primebds import PrimeBDS

RANKS = list(load_permissions().keys())
PERMISSIONS = load_permissions()

MANAGED_PERMISSIONS_LIST = []

MINECRAFT_PERMISSIONS = [
    "minecraft",
    "minecraft.command",
    "minecraft.command.aimassist",
    "minecraft.command.allowlist",
    "minecraft.command.camera",
    "minecraft.command.camerashake",
    "minecraft.command.clear",
    "minecraft.command.clearspawnpoint",
    "minecraft.command.clone",
    "minecraft.command.controlscheme",
    "minecraft.command.damage",
    "minecraft.command.daylock",
    "minecraft.command.deop",
    "minecraft.command.dialogue",
    "minecraft.command.effect",
    "minecraft.command.enchant",
    "minecraft.command.event",
    "minecraft.command.execute",
    "minecraft.command.fill",
    "minecraft.command.function",
    "minecraft.command.fog",
    "minecraft.command.gamemode",
    "minecraft.command.gametest",
    "minecraft.command.gamerule",
    "minecraft.command.give",
    "minecraft.command.help",
    "minecraft.command.inputpermission",
    "minecraft.command.kill",
    "minecraft.command.locate",
    "minecraft.command.list",
    "minecraft.command.loot",
    "minecraft.command.music",
    "minecraft.command.me",
    "minecraft.command.mobevent",
    "minecraft.command.op",
    "minecraft.command.particle",
    "minecraft.command.place",
    "minecraft.command.playanimation",
    "minecraft.command.playsound",
    "minecraft.command.permissionslist",
    "minecraft.command.recipe",
    "minecraft.command.replaceitem",
    "minecraft.command.ride",
    "minecraft.command.reload",
    "minecraft.command.save",
    "minecraft.command.say",
    "minecraft.command.schedule",
    "minecraft.command.scoreboard",
    "minecraft.command.script",
    "minecraft.command.scriptevent",
    "minecraft.command.sendshowstoreoffer",
    "minecraft.command.setblock",
    "minecraft.command.setworldspawn",
    "minecraft.command.spawnpoint",
    "minecraft.command.spreadplayers",
    "minecraft.command.stop",
    "minecraft.command.stopsound",
    "minecraft.command.tag",
    "minecraft.command.teleport",
    "minecraft.command.tell",
    "minecraft.command.tellraw",
    "minecraft.command.time",
    "minecraft.command.title",
    "minecraft.command.titleraw",
    "minecraft.command.tickingarea",
    "minecraft.command.transfer",
    "minecraft.command.toggledownfall",
    "minecraft.command.weather",
    "minecraft.command.wsserver",
    "minecraft.command.xp",
    "minecraft.command.summon",
    "minecraft.command.structure",
    "minecraft.command.testfor",
    "minecraft.command.testforblock",
    "minecraft.command.testforblocks",
    "minecraft.command.transfer",
    "minecraft.command.ride",
]

EXTRA_PERMS = [
    "primebds.exempt.msgtoggle",
    "primebds.exempt.globalmute",
    "primebds.exempt.mute",
    "primebds.exempt.ban",
    "primebds.exempt.kick",
    "primebds.exempt.warn",
    "primebds.exempt.homes",
    "primebds.exempt.homes.other",
    "primebds.exempt.home.delays",
    "primebds.exempt.home.cooldowns",
    "primebds.exempt.warp.delays",
    "primebds.exempt.warp.cooldowns",
    "primebds.exempt.spawn.delays",
    "primebds.exempt.spawn.cooldowns",
    "primebds.exempt.back.delays",
    "primebds.exempt.back.cooldowns",
    "primebds.command.heal.other",
    "primebds.command.feed.other",
    "primebds.command.repair.other",
    "primebds.command.god.other",
    "primebds.command.hat.other"
]

def reload_rank_list():
    global RANKS
    RANKS = list(load_permissions().keys())

def get_ranks() -> list[str]:
    return RANKS

def load_perms(self: "PrimeBDS"):
    config = load_config()
    modules = config.get("modules", {})
    perms_manager = modules.get("permissions_manager", {})
    minecraft_enabled = perms_manager.get("minecraft", True)
    primebds_enabled = perms_manager.get("primebds", True)
    endstone_enabled = perms_manager.get("endstone", True)
    wildcard_enabled = perms_manager.get("*", True)

    can_count = True
    plugin_perms = set()
    try:
        for plugin in self.server.plugin_manager.plugins:
            plugin_perm_set = set()

            data = getattr(plugin, "_get_description", lambda: None)()
            if not data:
                continue
            
            perms = getattr(data, "permissions", [])
            commands = getattr(data, "commands", [])

            if perms:
                for attachment in perms:
                    perm_lower = attachment.name.lower()
                    if perm_lower in {"endstone.command.banip", "endstone.command.unbanip", "endstone.command.banlist"}:
                        continue

                    prefix = perm_lower.split(".")[0]
                    if (prefix == "minecraft" and minecraft_enabled) or \
                    (prefix == "primebds" and primebds_enabled) or \
                    (prefix == "endstone" and endstone_enabled) or \
                    (prefix not in {"minecraft", "primebds", "endstone"} and wildcard_enabled):
                        plugin_perms.add(perm_lower)
                        plugin_perm_set.add(perm_lower)
            elif commands and wildcard_enabled:
                for cmd in commands:
                    perms = cmd.permissions
                    for perm in perms:
                        perm_lower = perm.lower()
                        plugin_perms.add(perm_lower)
                        plugin_perm_set.add(perm_lower)

            if getattr(plugin, "name", "") == "primebds":
                for perm in EXTRA_PERMS:
                    plugin_perms.add(perm)
                    plugin_perm_set.add(perm)

            plugin_name = getattr(plugin, "name", None)
            if plugin_name == None:
                if plugin_perm_set:
                    first_perm = next(iter(plugin_perm_set))
                    plugin_name = first_perm.split(".")[0]
                else:
                    plugin_name = "unnamed plugin"

            if plugin_perm_set:
                first_prefix = next(iter(plugin_perm_set)).split(".")[0]
                if (first_prefix == "minecraft" and minecraft_enabled) or \
                    (first_prefix == "primebds" and primebds_enabled) or \
                    (first_prefix == "endstone" and endstone_enabled) or \
                    (first_prefix not in {"minecraft", "primebds", "endstone"} and wildcard_enabled):
                    print(f"[PrimeBDS] {plugin_name}: Loaded {len(plugin_perm_set)} permissions")
    except Exception as e:
        print(f"[PrimeBDS] An error occured when scanning plugins... (RTTI error caused by a plugin). This will not affect permissions. However, permissions for plugins may not be listed in /permissionslist")
        can_count = False

    server_registered = {str(p.name).lower() for p in self.server.plugin_manager.permissions}

    for perm in server_registered:
        prefix = perm.split(".")[0]
        if (prefix == "minecraft" and minecraft_enabled) or \
        (prefix == "primebds" and primebds_enabled) or \
        (prefix == "endstone" and endstone_enabled) or \
        (prefix not in {"minecraft", "endstone"} and wildcard_enabled):
            plugin_perms.add(perm)

    if minecraft_enabled:
        plugin_perms |= {p.lower() for p in MINECRAFT_PERMISSIONS}

    MANAGED_PERMISSIONS_LIST.clear()
    MANAGED_PERMISSIONS_LIST.extend(plugin_perms)

    for perm in EXTRA_PERMS:
        if perm not in MANAGED_PERMISSIONS_LIST and primebds_enabled:
            MANAGED_PERMISSIONS_LIST.append(perm)

    endstone_filtered = [perm for perm in plugin_perms if "endstone" in perm]
    if endstone_filtered:
        print(f"[PrimeBDS] endstone: Loaded {len(endstone_filtered)} permissions")

    minecraft_filtered = [perm for perm in plugin_perms if perm in {p.lower() for p in MINECRAFT_PERMISSIONS}]
    print(f"[PrimeBDS] minecraft: Loaded {len(minecraft_filtered)} permissions")

    if can_count:
        print(f"[PrimeBDS] Total managed permissions: {len(MANAGED_PERMISSIONS_LIST)}")
    else:
        print(f"[PrimeBDS] Total managed permissions: {len(MANAGED_PERMISSIONS_LIST)}+ (Estimate)")
        print(f"[PrimeBDS] Some permissions may be missing due to RTTI error")

def normalize_rank_name(rank: str) -> str:
    rank = rank.lower()
    if rank.startswith("primebds.rank."):
        rank = rank[len("primebds.rank."):]

    for r in RANKS:
        if r.lower() == rank:
            return r
        
    return "Default"

def get_rank_group(rank_name: str) -> dict | None:
    rank_lower = rank_name.lower()
    for key, value in PERMISSIONS.items():
        if key.lower() == rank_lower:
            return value
    return None

def check_rank_exists(self: "PrimeBDS", target: Player, rank: str) -> str:
    """Ensure a rank exists; fallback to Default if missing or invalid. Case-insensitive."""
    try:
        ranks_map = {r.lower(): r for r in PERMISSIONS.keys()}

        rank_lower = rank.lower()
        if rank_lower not in ranks_map:
            raise ValueError("Rank missing or invalid")

        proper_rank = ranks_map[rank_lower]
        group = get_rank_group(proper_rank)

        if not isinstance(group, dict) or "permissions" not in group or not isinstance(group["permissions"], (dict, list)):
            raise ValueError("Rank permissions invalid")

        return proper_rank
    except Exception as e:
        print(f"Invalid rank '{rank}' for {target.name}, resetting to Default: {e}")
        self.db.update_user_data(target.name, 'internal_rank', "Default")
        return "Default"

def get_permission_header(plugin) -> str | None:
    """
    Returns the first segment of a permission if the plugin has a permissions header
    """

    data = getattr(plugin, "_get_description", lambda: None)()
    if not data:
        return None

    perms = getattr(data, "permissions", [])
    if not perms:
        return None
    
    for attachment in perms:
        perm_lower = attachment.name.lower()
        break

    return perm_lower.split(".")[0]

def get_rank_permissions(rank: str) -> dict[str, bool]:
    """Return a dict of permissions (lowercased) -> bool for the given rank, including inheritance."""
    global PERMISSIONS
    PERMISSIONS = load_permissions(None, True)

    base_rank = normalize_rank_name(rank)
    seen_ranks = set()

    result: dict[str, bool] = {p.lower(): False for p in MANAGED_PERMISSIONS_LIST}

    def gather_permissions(r):
        r_norm = normalize_rank_name(r)
        if r_norm in seen_ranks:
            return
        seen_ranks.add(r_norm)

        group = None
        for key, val in PERMISSIONS.items():
            if key.lower() == r_norm.lower():
                group = val
                break
        if not group or not isinstance(group, dict):
            return

        inherits = group.get("inherits", [])
        if isinstance(inherits, list):
            for parent in inherits:
                if isinstance(parent, str):
                    gather_permissions(parent)

        perms = group.get("permissions", {})
        if isinstance(perms, dict):
            fixed_perms = {k.lower(): bool(v) for k, v in perms.items()}
        elif isinstance(perms, list):
            fixed_perms = {p.lower(): True for p in perms if isinstance(p, str)}
        else:
            fixed_perms = {}

        if "*" in fixed_perms:
            wildcard_value = fixed_perms["*"]
            for p in list(result.keys()):
                if p not in fixed_perms:
                    result[p] = wildcard_value

        for perm_name, allowed in fixed_perms.items():
            if perm_name == "*":
                continue
            result[perm_name] = allowed

    gather_permissions(base_rank)
    return result

perm_cache = {}
def check_perms(self: "PrimeBDS", player_or_user, perm: str, check_rank=False) -> bool:
    now = time.time()
    xuid = getattr(player_or_user, "xuid", None)

    if hasattr(player_or_user, "has_permission") and not check_rank:
        result = player_or_user.has_permission(perm)
        return result

    if xuid is None:
        return False

    cached = perm_cache.get(xuid)
    if cached:
        perms, ts = cached
        result = perms.get(perm, False)
        return result

    user_name = self.db.get_name_by_xuid(xuid)
    user = self.db.get_offline_user(user_name)
    if not user:
        return False

    rank = getattr(user, "internal_rank", "") if hasattr(user, "internal_rank") else ""
    final_perms = get_rank_permissions(rank)

    user_permissions = self.db.get_permissions(xuid)
    for perm_name, allowed in user_permissions.items():
        final_perms[perm_name.lower()] = bool(allowed)

    perm_cache[xuid] = (final_perms, now)
    result = final_perms.get(perm.lower(), False)
    return result

def invalidate_perm_cache(xuid: str):
    perm_cache.pop(xuid, None)

_prefix_cache = {}
_suffix_cache = {}
def get_prefix(rank: str, permissions=None) -> str:
    """
    Returns the prefix for a given rank, using cache if available.
    """
    global _prefix_cache
    if rank in _prefix_cache:
        return _prefix_cache[rank]

    if permissions is None:
        permissions = load_permissions(cache=True)

    prefix = permissions.get(rank, {}).get("prefix", "")
    _prefix_cache[rank] = prefix
    return prefix

def get_suffix(rank: str, permissions=None) -> str:
    """
    Returns the suffix for a given rank, using cache if available.
    """
    global _suffix_cache
    if rank in _suffix_cache:
        return _suffix_cache[rank]

    if permissions is None:
        permissions = load_permissions(cache=True)

    suffix = permissions.get(rank, {}).get("suffix", "")
    _suffix_cache[rank] = suffix
    return suffix

def clear_prefix_suffix_cache():
    """
    Clears the prefix/suffix caches (useful if permissions are reloaded).
    """
    global _prefix_cache, _suffix_cache
    _prefix_cache.clear()
    _suffix_cache.clear()
            
def check_internal_rank(user1_rank: str, user2_rank: str) -> bool:
    if user1_rank not in RANKS or user2_rank not in RANKS:
        return False
    return RANKS.index(user1_rank) < RANKS.index(user2_rank)
