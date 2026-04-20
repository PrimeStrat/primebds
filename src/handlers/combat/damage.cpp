/// @file damage.cpp
/// Damage event handler with god mode, custom damage modifiers, and hit cooldowns.

#include "primebds/handlers/combat/damage.h"
#include "primebds/handlers/combat/tag_overrides.h"
#include "primebds/plugin.h"
#include "primebds/utils/config/config_manager.h"

#include <chrono>
#include <string>

namespace primebds::handlers::combat {

    void handleDamageEvent(PrimeBDS &plugin, endstone::ActorDamageEvent &event) {
        auto &entity = event.getActor();
        std::string entity_key = std::string(entity.getType()) + ":" + std::to_string(entity.getRuntimeId());
        auto now = std::chrono::duration<double>(
                       std::chrono::steady_clock::now().time_since_epoch())
                       .count();
        double last_hit_time = 0.0;
        auto it = plugin.entity_damage_cooldowns.find(entity_key);
        if (it != plugin.entity_damage_cooldowns.end())
            last_hit_time = it->second;

        auto cfg = config::ConfigManager::instance().config();
        auto &source = event.getDamageSource();
        std::string damage_type = std::string(source.getType());
        std::vector<std::string> source_tags;

        if (auto *src_actor = source.getActor()) {
            auto tags = src_actor->getScoreboardTags();
            source_tags.assign(tags.begin(), tags.end());
        }

        // God mode check (player only)
        auto *player = dynamic_cast<endstone::Player *>(&entity);
        if (player) {
            if (plugin.isgod.count(player->getUniqueId().str())) {
                event.setCancelled(true);
                return;
            }
        }

        // Tag-based overrides
        auto modifier = getCustomTag(cfg, source_tags, "base_damage");
        auto kb_cooldown = getCustomTag(cfg, source_tags, "hit_cooldown_in_seconds");
        auto fall_height = getCustomTag(cfg, source_tags, "fall_damage_height");
        auto no_fire = getCustomTag(cfg, source_tags, "disable_fire_damage");
        auto no_explosion = getCustomTag(cfg, source_tags, "disable_explosion_damage");

        // Fire damage disable
        if (no_fire.has_value() && no_fire.value() != 0.0 &&
            (damage_type == "fire_tick" || damage_type == "fire" || damage_type == "lava")) {
            event.setCancelled(true);
            return;
        }

        // Explosion damage disable
        if (no_explosion.has_value() && no_explosion.value() != 0.0 &&
            damage_type == "entity_explosive") {
            event.setCancelled(true);
            return;
        }

        // Custom fall damage height
        if (damage_type == "fall" && fall_height.has_value() && fall_height.value() != 3.5) {
            if (event.getDamage() * 2.0 < fall_height.value()) {
                event.setCancelled(true);
                return;
            }
        }

        // Custom base damage modifier
        if (modifier.has_value() && modifier.value() != 1.0)
            event.setDamage(event.getDamage() + modifier.value());

        plugin.entity_last_hit[entity_key] = damage_type;

        // Hit cooldown enforcement (only if configured)
        if (kb_cooldown.has_value()) {
            double cd = kb_cooldown.value();
            if (now - last_hit_time < cd && damage_type == "entity_attack")
                event.setCancelled(true);
        }
    }

} // namespace primebds::handlers::combat
