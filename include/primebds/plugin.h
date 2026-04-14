/// @file plugin.h
/// PrimeBDS main plugin class.
///
/// An essentials plugin for diagnostics, stability, and quality of life
/// on Minecraft Bedrock Edition.

#pragma once

#include "primebds/commands/command_registry.h"
#include "primebds/utils/database/server_db.h"
#include "primebds/utils/database/session_db.h"
#include "primebds/utils/database/user_db.h"
#include "primebds/utils/intervals.h"

// Handler includes (split organization)
#include "primebds/handlers/actions.h"
#include "primebds/handlers/afk.h"
#include "primebds/handlers/chat.h"
#include "primebds/handlers/gamerules.h"
#include "primebds/handlers/items.h"
#include "primebds/handlers/combat/damage.h"
#include "primebds/handlers/combat/knockback.h"
#include "primebds/handlers/connections/join.h"
#include "primebds/handlers/connections/kick.h"
#include "primebds/handlers/connections/leave.h"
#include "primebds/handlers/connections/login.h"
#include "primebds/handlers/preprocesses/command_intercept.h"

#include <endstone/endstone.hpp>
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <unordered_map>

namespace primebds {

    class EventListener;

    class PrimeBDS : public endstone::Plugin {
    public:
        void onLoad() override;
        void onEnable() override;
        void onDisable() override;
        bool onCommand(endstone::CommandSender &sender, const endstone::Command &command,
                       const std::vector<std::string> &args) override;

        // Re-apply custom permissions for a player
        void reloadCustomPerms(endstone::Player &player);

        // Check for stale sessions from unclean shutdown
        void checkForInactiveSessions();

        // --- Plugin state ---

        // Databases
        std::unique_ptr<db::UserDB> db;
        std::unique_ptr<db::SessionDB> sldb;
        std::unique_ptr<db::ServerDB> serverdb;

        // Gamerules cache
        std::map<std::string, int> gamerules;

        // Player state tracking
        std::set<std::string> cached_players;
        std::map<std::string, bool> vanish_state;
        std::map<std::string, bool> afk_cache;
        std::set<std::string> silentmutes;
        std::set<std::string> isgod;
        std::set<std::string> crasher_patch_applied;

        // Chat cooldown (player id -> last chat timestamp)
        std::map<std::string, double> chat_cooldown;

        // Global mute
        int globalmute = 0;

        // Combat tracking
        std::map<std::string, double> entity_damage_cooldowns;
        std::map<std::string, std::string> entity_last_hit;
        std::map<std::string, double> entity_enchant_hit;

        // Monitor/blockscan intervals
        std::map<std::string, int> monitor_intervals;
        std::map<std::string, int> blockscan_intervals;

        // Last shutdown time
        int64_t last_shutdown_time = 0;

        // AFK interval manager
        utils::IntervalManager afk_interval;

    private:
        std::unique_ptr<EventListener> listener_;
    };

    /// Event listener that delegates to handler functions.
    class EventListener {
    public:
        explicit EventListener(PrimeBDS &plugin) : plugin_(plugin) {}

        void onPlayerDeath(endstone::PlayerDeathEvent &event);
        void onPlayerTeleport(endstone::PlayerTeleportEvent &event);
        void onPlayerBedEnter(endstone::PlayerBedEnterEvent &event);
        void onPlayerEmote(endstone::PlayerEmoteEvent &event);
        void onPlayerSkinChange(endstone::PlayerSkinChangeEvent &event);
        void onLeavesDecay(endstone::LeavesDecayEvent &event);
        void onPlayerGameModeChange(endstone::PlayerGameModeChangeEvent &event);
        void onPlayerInteractActor(endstone::PlayerInteractActorEvent &event);
        void onItemPickup(endstone::PlayerPickupItemEvent &event);
        void onEntityDamage(endstone::ActorDamageEvent &event);
        void onEntityKnockback(endstone::ActorKnockbackEvent &event);
        void onPlayerLogin(endstone::PlayerLoginEvent &event);
        void onPlayerJoin(endstone::PlayerJoinEvent &event);
        void onPlayerQuit(endstone::PlayerQuitEvent &event);
        void onPlayerKick(endstone::PlayerKickEvent &event);
        void onPlayerCommand(endstone::PlayerCommandEvent &event);
        void onServerCommand(endstone::ServerCommandEvent &event);
        void onPlayerChat(endstone::PlayerChatEvent &event);
        void onServerLoad(endstone::ServerLoadEvent &event);

    private:
        PrimeBDS &plugin_;
    };

} // namespace primebds
