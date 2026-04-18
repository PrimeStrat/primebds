#include "primebds/plugin.h"
#include "primebds/commands/command_registry.h"
#include "primebds/utils/config/config_manager.h"
#include "primebds/utils/database/user_db.h"
#include "primebds/utils/database/session_db.h"
#include "primebds/utils/database/server_db.h"
#include "primebds/utils/permissions/permission_manager.h"
#include "primebds/utils/logging.h"

#include "primebds/commands/command_metadata.h"

#include <algorithm>
#include <ctime>
#include <filesystem>
#include <set>

namespace primebds {

    // ---------------------------------------------------------------------------
    // Plugin lifecycle
    // ---------------------------------------------------------------------------

    void PrimeBDS::onLoad() {
        getLogger().info("PrimeBDS v{} loading...", getDescription().getVersion());

        // Ensure data directory exists
        std::filesystem::create_directories(getDataFolder());

        // Create subdirectories
        auto db_dir = getDataFolder() / "database";
        auto profiles_dir = getDataFolder() / "allowlist_profiles";
        std::filesystem::create_directories(db_dir);
        std::filesystem::create_directories(profiles_dir);

        // Point ConfigManager at Endstone's data folder
        config::ConfigManager::setDataFolder(getDataFolder().string());

        // Initialise databases
        db = std::make_unique<db::UserDB>((db_dir / "users.db").string());
        sldb = std::make_unique<db::SessionDB>((db_dir / "sessions.db").string());
        serverdb = std::make_unique<db::ServerDB>((db_dir / "server.db").string());

        // Load configuration
        config::ConfigManager::instance().load();

        last_shutdown_time = (int64_t)std::time(nullptr);
    }

    void PrimeBDS::onEnable() {
        getLogger().info("PrimeBDS v{} enabled.", getDescription().getVersion());

        // Register event listener
        listener_ = std::make_unique<EventListener>(*this);
        registerEvent(&EventListener::onPlayerLogin, *listener_);
        registerEvent(&EventListener::onPlayerJoin, *listener_);
        registerEvent(&EventListener::onPlayerQuit, *listener_);
        registerEvent(&EventListener::onPlayerKick, *listener_);
        registerEvent(&EventListener::onPlayerChat, *listener_);
        registerEvent(&EventListener::onPlayerCommand, *listener_);
        registerEvent(&EventListener::onServerCommand, *listener_);
        registerEvent(&EventListener::onPlayerDeath, *listener_);
        registerEvent(&EventListener::onEntityDamage, *listener_);
        registerEvent(&EventListener::onEntityKnockback, *listener_);
        registerEvent(&EventListener::onPlayerGameModeChange, *listener_);
        registerEvent(&EventListener::onPlayerInteractActor, *listener_);
        registerEvent(&EventListener::onServerLoad, *listener_);

        // Commands are declared in the ENDSTONE_PLUGIN macro body below;
        // onCommand() dispatches to our CommandRegistry handlers.

        // Generate / sync commands.json
        config::ConfigManager::instance().loadCommandConfig();

        // Check for inactive sessions from unclean shutdown
        checkForInactiveSessions();
    }

    void PrimeBDS::onDisable() {
        getLogger().info("PrimeBDS v{} disabled.", getDescription().getVersion());

        // End all active sessions
        for (auto *player : getServer().getOnlinePlayers()) {
            sldb->endSession(player->getXuid());
            // Save logout position
            auto loc = player->getLocation();
            std::string pos_str = std::to_string(loc.getX()) + "," +
                                  std::to_string(loc.getY()) + "," +
                                  std::to_string(loc.getZ()) + "," +
                                  player->getDimension().getName();
            db->updateUser(player->getXuid(), "last_logout_pos", pos_str);
        }

        // Save config
        config::ConfigManager::instance().save();

        last_shutdown_time = (int64_t)std::time(nullptr);
    }

    // ---------------------------------------------------------------------------
    // Command dispatch
    // ---------------------------------------------------------------------------

    bool PrimeBDS::onCommand(endstone::CommandSender &sender,
                             const endstone::Command &command,
                             const std::vector<std::string> &args) {
        auto &registry = CommandRegistry::instance();
        auto *reg = registry.find(command.getName());
        if (reg) {
            if (!config::ConfigManager::instance().isCommandEnabled(command.getName())) {
                sender.sendMessage("\u00a7cThis command is disabled.");
                return false;
            }
            return reg->handler(*this, sender, args);
        }
        sender.sendMessage("\u00a7cUnknown command.");
        return false;
    }

    // ---------------------------------------------------------------------------
    // Helpers
    // ---------------------------------------------------------------------------

    void PrimeBDS::reloadCustomPerms(endstone::Player &player) {
        auto &pm = permissions::PermissionManager::instance();
        auto user = db->getOnlineUser(player.getXuid());
        if (!user) {
            // Player not in DB yet — save them first
            db->saveUser(player.getXuid(), player.getUniqueId().str(),
                         player.getName(), static_cast<int>(player.getPing().count()),
                         player.getDeviceOS(), player.getDeviceId(),
                         static_cast<int64_t>(player.getRuntimeId()),
                         player.getGameVersion());
            user = db->getOnlineUser(player.getXuid());
            if (!user)
                return;
        }

        std::string internal_rank = pm.checkRankExists(*this, player, user->internal_rank);
        auto rank_permissions = pm.getRankPermissions(internal_rank);
        auto user_permissions = db->getPermissions(player.getXuid());
        auto &managed_perms = pm.MANAGED_PERMISSIONS_LIST;

        // Linked permission groups — if any in the group are true, all become true
        static const std::vector<std::vector<std::string>> linked_groups = {
            {"primebds.command.permban", "endstone.command.ban"},
            {"primebds.command.ipban", "endstone.command.banip"},
            {"primebds.command.removeban", "endstone.command.unban", "endstone.command.unbanip"},
            {"primebds.command.filterlist", "endstone.command.banlist"}};

        // Build final permission map: start all managed perms as false
        std::map<std::string, bool> final_permissions;
        for (auto &p : managed_perms)
            final_permissions[p] = false;

        // Layer rank permissions
        for (auto &[perm, allowed] : rank_permissions)
            final_permissions[perm] = allowed;

        // Layer user-specific overrides (highest priority)
        for (auto &[perm, allowed] : user_permissions)
            final_permissions[perm] = allowed;

        // Apply linked groups
        for (auto &group : linked_groups) {
            bool seen_true = false;
            bool seen_false = false;
            for (auto &perm : group) {
                auto it = final_permissions.find(perm);
                if (it != final_permissions.end()) {
                    if (it->second)
                        seen_true = true;
                    else
                        seen_false = true;
                }
            }
            if (seen_true || seen_false) {
                bool group_value = seen_true;
                for (auto &perm : group)
                    final_permissions[perm] = group_value;
            }
        }

        // Remove any existing primebdsoverride attachment
        for (auto *info : player.getEffectivePermissions()) {
            if (info->getPermission() == "primebdsoverride") {
                auto *att = info->getAttachment();
                if (att)
                    att->remove();
            }
        }

        // Create new attachment and apply all permissions
        auto *attachment = player.addAttachment(*this, "primebdsoverride", true);
        if (!attachment)
            return;

        // Detect plugin-star overrides (e.g. "minecraft" or "minecraft.command")
        static const std::set<std::string> internal_perms = {
            "minecraft", "minecraft.command", "endstone", "endstone.command"};

        std::map<std::string, bool> plugin_stars;
        for (auto &[perm, value] : final_permissions) {
            if (internal_perms.count(perm))
                continue;
            auto dot = perm.find('.');
            std::string prefix = (dot != std::string::npos) ? perm.substr(0, dot) : perm;
            std::string cmd_key = prefix + ".command";
            if (prefix == perm || cmd_key == perm)
                plugin_stars[prefix] = value;
        }

        // Apply permissions
        for (auto &[perm, value] : final_permissions) {
            if (internal_perms.count(perm))
                continue;
            auto dot = perm.find('.');
            std::string prefix = (dot != std::string::npos) ? perm.substr(0, dot) : perm;
            auto star_it = plugin_stars.find(prefix);
            if (star_it != plugin_stars.end())
                attachment->setPermission(perm, star_it->second);
            else
                attachment->setPermission(perm, value);
        }

        // Auto op/deop based on rank
        auto rank_lower = internal_rank;
        std::transform(rank_lower.begin(), rank_lower.end(), rank_lower.begin(), ::tolower);
        if (rank_lower == "operator" && !player.isOp() && player.isValid()) {
            (void)getServer().dispatchCommand(getServer().getCommandSender(),
                                              "op \"" + user->name + "\"");
        } else if (rank_lower != "operator" && player.isOp() && player.isValid()) {
            (void)getServer().dispatchCommand(getServer().getCommandSender(),
                                              "deop \"" + user->name + "\"");
        }

        player.updateCommands();
        player.recalculatePermissions();
        pm.clearPrefixSuffixCache();
        pm.invalidatePermCache(player.getXuid());
    }

    void PrimeBDS::checkForInactiveSessions() {
        // End any sessions that are still open (from unclean shutdown)
        auto active = sldb->getActiveSessions();
        for (auto &session : active) {
            sldb->endSession(session["xuid"]);
        }
    }

    // ---------------------------------------------------------------------------
    // EventListener delegation
    // ---------------------------------------------------------------------------

    void EventListener::onPlayerDeath(endstone::PlayerDeathEvent &event) {
        handlers::handleDeathEvent(plugin_, event);
    }

    void EventListener::onPlayerTeleport(endstone::PlayerTeleportEvent &event) {
        handlers::handleTeleportEvent(plugin_, event);
    }

    void EventListener::onPlayerBedEnter(endstone::PlayerBedEnterEvent &event) {
        handlers::handleBedEnterEvent(plugin_, event);
    }

    void EventListener::onPlayerEmote(endstone::PlayerEmoteEvent &event) {
        handlers::handleEmoteEvent(plugin_, event);
    }

    void EventListener::onPlayerSkinChange(endstone::PlayerSkinChangeEvent &event) {
        handlers::handleSkinChangeEvent(plugin_, event);
    }

    void EventListener::onLeavesDecay(endstone::LeavesDecayEvent &event) {
        handlers::handleLeavesDecayEvent(plugin_, event);
    }

    void EventListener::onPlayerGameModeChange(endstone::PlayerGameModeChangeEvent &event) {
        handlers::handleGamemodeEvent(plugin_, event);
    }

    void EventListener::onPlayerInteractActor(endstone::PlayerInteractActorEvent &event) {
        handlers::handleInteractEvent(plugin_, event);
    }

    void EventListener::onItemPickup(endstone::PlayerPickupItemEvent &event) {
        handlers::handleItemPickupEvent(plugin_, event);
    }

    void EventListener::onEntityDamage(endstone::ActorDamageEvent &event) {
        handlers::combat::handleDamageEvent(plugin_, event);
    }

    void EventListener::onEntityKnockback(endstone::ActorKnockbackEvent &event) {
        handlers::combat::handleKnockbackEvent(plugin_, event);
    }

    void EventListener::onPlayerLogin(endstone::PlayerLoginEvent &event) {
        handlers::connections::handleLoginEvent(plugin_, event);
    }

    void EventListener::onPlayerJoin(endstone::PlayerJoinEvent &event) {
        handlers::connections::handleJoinEvent(plugin_, event);
    }

    void EventListener::onPlayerQuit(endstone::PlayerQuitEvent &event) {
        handlers::connections::handleLeaveEvent(plugin_, event);
    }

    void EventListener::onPlayerKick(endstone::PlayerKickEvent &event) {
        handlers::connections::handleKickEvent(plugin_, event);
    }

    void EventListener::onPlayerCommand(endstone::PlayerCommandEvent &event) {
        handlers::preprocesses::handleCommandPreprocess(plugin_, event);
    }

    void EventListener::onServerCommand(endstone::ServerCommandEvent &event) {
        handlers::preprocesses::handleServerCommandPreprocess(plugin_, event);
    }

    void EventListener::onPlayerChat(endstone::PlayerChatEvent &event) {
        handlers::handleChatEvent(plugin_, event);
    }

    void EventListener::onServerLoad(endstone::ServerLoadEvent &event) {
        handlers::handleServerLoadEvent(plugin_, event);
    }

} // namespace primebds

// ---------------------------------------------------------------------------
// Endstone plugin entry point
// ---------------------------------------------------------------------------

ENDSTONE_PLUGIN("primebds", "3.4.1", primebds::PrimeBDS) {
    description = "An essentials plugin for diagnostics, stability, and quality of life on Minecraft Bedrock Edition.";
    authors = {"PrimeStrat"};

    // All command metadata is defined in command_metadata.cpp.
    // Update that file when adding, removing, or changing commands.
    primebds::registerEndstoneCommands(*this);
}