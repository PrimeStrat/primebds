/// @file knockback.cpp
/// Knockback event handler with custom modifiers and sprint/projectile handling.

#include "primebds/handlers/combat/knockback.h"
#include "primebds/handlers/combat/tag_overrides.h"
#include "primebds/plugin.h"
#include "primebds/utils/config/config_manager.h"

#include <chrono>
#include <cmath>

namespace primebds::handlers::combat
{

    void handleKnockbackEvent(PrimeBDS &plugin, endstone::ActorKnockbackEvent &event)
    {
        auto cfg = config::ConfigManager::instance().config();
        auto *source = event.getSource();
        std::string entity_key = std::string(event.getActor().getType()) + ":" +
                                 std::to_string(event.getActor().getRuntimeId());

        auto last_hit_it = plugin.entity_last_hit.find(entity_key);
        std::string last_hit_type = (last_hit_it != plugin.entity_last_hit.end()) ? last_hit_it->second : "";

        auto *source_player = source ? dynamic_cast<endstone::Player *>(source) : nullptr;
        std::vector<std::string> tags;
        if (source_player)
        {
            auto stags = source_player->getScoreboardTags();
            tags.assign(stags.begin(), stags.end());
        }

        auto kb_cooldown_opt = getCustomTag(cfg, tags, "hit_cooldown_in_seconds");
        double kb_cooldown = kb_cooldown_opt.value_or(0.45);
        auto now = std::chrono::duration<double>(
                       std::chrono::steady_clock::now().time_since_epoch())
                       .count();
        double last_hit_time = 0.0;
        auto cd_it = plugin.entity_damage_cooldowns.find(entity_key);
        if (cd_it != plugin.entity_damage_cooldowns.end())
            last_hit_time = cd_it->second;

        // Cooldown tracking
        if (now - last_hit_time >= kb_cooldown && last_hit_type == "entity_attack")
        {
            plugin.entity_damage_cooldowns[entity_key] = now;
        }
        else if (now - last_hit_time < kb_cooldown && last_hit_type == "entity_attack")
        {
            // Check enchant knockback
            if (source_player)
            {
                auto held = source_player->getInventory().getItemInMainHand();
                if (held && held->getType() != endstone::ItemType::Air)
                {
                    auto meta = held->getItemMeta();
                    int kb_lvl = meta->getEnchantLevel(endstone::EnchantmentId("knockback"));
                    auto enc_it = plugin.entity_enchant_hit.find(entity_key);
                    double last_enc = (enc_it != plugin.entity_enchant_hit.end()) ? enc_it->second : 0.0;
                    if (kb_lvl > 0 && now - last_enc >= kb_cooldown)
                    {
                        plugin.entity_enchant_hit[entity_key] = last_hit_time;
                        plugin.entity_last_hit[entity_key] = "";
                        return;
                    }
                }
            }
            plugin.entity_last_hit[entity_key] = "";
            event.setCancelled(true);
            return;
        }

        auto kb = event.getKnockback();

        // Projectile knockback
        if (last_hit_type == "projectile")
        {
            auto h_proj = getCustomTag(cfg, tags, "projectiles.horizontal_knockback_modifier");
            auto v_proj = getCustomTag(cfg, tags, "projectiles.vertical_knockback_modifier");

            if (!h_proj.has_value() && !v_proj.has_value())
                return;

            double hm = h_proj.value_or(1.0);
            double vm = v_proj.value_or(1.0);

            double nx = kb.getX() * hm;
            double ny = kb.getY() * vm;
            double nz = kb.getZ() * hm;

            event.setKnockback({nx, std::abs(ny), nz});
            return;
        }

        // Normal knockback modifiers
        auto kb_h = getCustomTag(cfg, tags, "horizontal_knockback_modifier");
        auto kb_v = getCustomTag(cfg, tags, "vertical_knockback_modifier");
        auto kb_sh = getCustomTag(cfg, tags, "horizontal_sprint_knockback_modifier");
        auto kb_sv = getCustomTag(cfg, tags, "vertical_sprint_knockback_modifier");
        auto disable_sprint = getCustomTag(cfg, tags, "disable_sprint_hits");

        if (!kb_h.has_value() && !kb_v.has_value() && !kb_sh.has_value() && !kb_sv.has_value())
            return;

        double h_mod = kb_h.value_or(1.0);
        double v_mod = kb_v.value_or(1.0);
        double sh_mod = kb_sh.value_or(1.0);
        double sv_mod = kb_sv.value_or(1.0);

        bool sprinting = source_player && source_player->isSprinting();

        if (sprinting && disable_sprint.has_value() && disable_sprint.value() != 0.0 && kb.getY() <= 0)
        {
            event.setCancelled(true);
            return;
        }

        double nx = kb.getX() * h_mod;
        double ny = kb.getY() * v_mod;
        double nz = kb.getZ() * h_mod;

        if (sprinting && sh_mod != 0.0)
        {
            nx *= sh_mod;
            nz *= sh_mod;
        }

        if (sprinting && kb.getY() < 0)
            ny = (ny * sv_mod) / 2.0;

        event.setKnockback({nx, std::abs(ny), nz});
    }

} // namespace primebds::handlers::combat
