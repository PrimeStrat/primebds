from typing import TYPE_CHECKING
from time import time

from endstone import Player
from endstone.util import Vector
from endstone.event import ActorDamageEvent, ActorKnockbackEvent

from endstone_primebds.utils.config_util import load_config

if TYPE_CHECKING:
    from endstone_primebds.primebds import PrimeBDS

def handle_damage_event(self: "PrimeBDS", ev: ActorDamageEvent):
    config = load_config()
    entity = ev.actor
    entity_key = f"{entity.type}:{entity.id}"
    current_time = time()
    last_hit_time = self.entity_damage_cooldowns.get(entity_key, 0)

    damage_source = getattr(ev, "damage_source", None)
    damage_type = getattr(damage_source, "type", None) if damage_source else None
    source_actor_tags = getattr(getattr(damage_source, "actor", None), "scoreboard_tags", []) or []

    if entity.type == "minecraft:player":
        if entity.id in self.isgod:
            ev.is_cancelled = True
            return
        if self.vanish_state.get(entity.unique_id, False):
            ev.is_cancelled = True
            return

    modifier = get_custom_tag(config, source_actor_tags, "base_damage")
    kb_cooldown = get_custom_tag(config, source_actor_tags, "hit_cooldown_in_seconds")
    fall_damage_height = get_custom_tag(config, source_actor_tags, "fall_damage_height")
    disable_fire_dmg = get_custom_tag(config, source_actor_tags, "disable_fire_damage")
    disable_explosion_dmg = get_custom_tag(config, source_actor_tags, "disable_explosion_damage")

    if damage_type:
        if disable_fire_dmg and damage_type in ("fire_tick", "fire", "lava"):
            ev.is_cancelled = True
            return
        if disable_explosion_dmg and damage_type == "entity_explosive":
            ev.is_cancelled = True
            return

    if damage_type == "fall" and fall_damage_height != 3.5 and ev.damage * 2 < fall_damage_height:
        ev.is_cancelled = True
        return

    if modifier != 1:
        ev.damage += modifier

    self.entity_last_hit[entity_key] = damage_type
    if current_time - last_hit_time < kb_cooldown and damage_type == "entity_attack":
        ev.is_cancelled = True

def handle_kb_event(self: "PrimeBDS", ev: ActorKnockbackEvent):

    config = load_config()
    source = ev.source
    source_player = self.server.get_player(source.name) if hasattr(source, "name") and source.name else None
    entity_key = f"{ev.actor.type}:{ev.actor.id}"
    last_hit_type = self.entity_last_hit.get(entity_key)
    tags = getattr(source_player, "scoreboard_tags", [])

    source_actor_tags = getattr(getattr(source, "actor", None), "scoreboard_tags", []) or []
    kb_cooldown = get_custom_tag(config, source_actor_tags, "hit_cooldown_in_seconds")
    current_time = time()
    last_hit_time = self.entity_damage_cooldowns.get(entity_key, 0)
    last_enchant_hit_time = self.entity_enchant_hit.get(entity_key, 0)

    if current_time - last_hit_time >= kb_cooldown and last_hit_type == "entity_attack":
        self.entity_damage_cooldowns[entity_key] = current_time
    elif current_time - last_hit_time < kb_cooldown and last_hit_type == "entity_attack":
        held_item = source_player.inventory.item_in_main_hand
        if held_item:
            kb_lvl = held_item.item_meta.get_enchant_level("knockback") 
            if kb_lvl > 0 and current_time - last_enchant_hit_time >= kb_cooldown:
                self.entity_enchant_hit[entity_key] = last_hit_time
                self.entity_last_hit[entity_key] = None
                return
            else:
                self.entity_last_hit[entity_key] = None
        ev.is_cancelled = True
        return
    
    if last_hit_type == "projectile":
        horizontal_proj_kb = get_custom_tag(config, tags, "projectiles.horizontal_knockback_modifier")
        vertical_proj_kb = get_custom_tag(config, tags, "projectiles.vertical_knockback_modifier")

        if all(
            modifier in (0, None)
            for modifier in (
                horizontal_proj_kb,
                vertical_proj_kb,
            )
        ):
            return

        horizontal_proj_kb = horizontal_proj_kb or 1.0
        vertical_proj_kb = vertical_proj_kb or 1.0

        newx = ev.knockback.x * horizontal_proj_kb
        newy = ev.knockback.y * vertical_proj_kb
        newz = ev.knockback.z * horizontal_proj_kb

        if ev.knockback.x == 0 or ev.knockback.z == 0:
            velocity = getattr(source, "velocity", Vector(0, 0, 0))
            newx = velocity.x * horizontal_proj_kb
            newz = velocity.z * horizontal_proj_kb

        ev.knockback = Vector(newx, abs(newy), newz)
        return

    kb_h_modifier = get_custom_tag(config, tags, "horizontal_knockback_modifier")
    kb_v_modifier = get_custom_tag(config, tags, "vertical_knockback_modifier")
    kb_sprint_h_modifier = get_custom_tag(config, tags, "horizontal_sprint_knockback_modifier")
    kb_sprint_v_modifier = get_custom_tag(config, tags, "vertical_sprint_knockback_modifier")
    disable_sprint_hits = get_custom_tag(config, tags, "disable_sprint_hits")

    # If all modifiers are 0, skip
    if all(
        modifier in (0, None)
        for modifier in (kb_h_modifier, kb_v_modifier, kb_sprint_h_modifier, kb_sprint_v_modifier)
    ):
        return

    # Use fallback default of 1.0 if not set
    kb_h_modifier = kb_h_modifier or 1.0
    kb_v_modifier = kb_v_modifier or 1.0
    kb_sprint_h_modifier = kb_sprint_h_modifier or 1.0
    kb_sprint_v_modifier = kb_sprint_v_modifier or 1.0

    is_player_sprinting = isinstance(source_player, Player) and getattr(source_player, "is_sprinting", False)

    # Sprint hit cancel logic (players only)
    if is_player_sprinting and disable_sprint_hits and ev.knockback.y <= 0:
        ev.is_cancelled = True
        return

    newx = ev.knockback.x * kb_h_modifier
    newy = ev.knockback.y * kb_v_modifier
    newz = ev.knockback.z * kb_h_modifier

    # Check for 0 kb on horizontal axes
    if ev.knockback.x == 0 or ev.knockback.z == 0:
        velocity = getattr(source_player or source, "velocity", Vector(0, 0, 0))
        newx = velocity.x * kb_h_modifier
        newz = velocity.z * kb_h_modifier

    if is_player_sprinting and kb_sprint_h_modifier != 0.0:
        newx *= kb_sprint_h_modifier
        newz *= kb_sprint_h_modifier

    if is_player_sprinting and ev.knockback.y < 0:
        newy = (newy * kb_sprint_v_modifier) / 2

    ev.knockback = Vector(newx, abs(newy), newz)

def get_custom_tag(config, tags, key):
    """
    Returns the custom KB modifiers, prioritizing tag-specific modifiers.
    Supports dot-separated nested keys (e.g. 'projectiles.horizontal_knockback_modifier').
    Falls back to global value if no tag match is found.
    """
    def deep_get(d, key_path):
        for k in key_path:
            if isinstance(d, dict) and k in d:
                d = d[k]
            else:
                return None
        return d

    key_path = key.split(".")

    # Global/default value
    default = deep_get(config["modules"]["combat"], key_path)

    tag_mods = config["modules"]["combat"].get("tag_overrides", {})
    for tag in tags:
        tag_override = tag_mods.get(tag, {})
        value = deep_get(tag_override, key_path)
        if value is not None:
            return value

    return default

