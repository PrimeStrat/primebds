/// @file plugin.cpp
/// PrimeBDS main plugin implementation — entry point, lifecycle, and command dispatch.

#include "primebds/plugin.h"
#include "primebds/commands/command_registry.h"
#include "primebds/utils/config/config_manager.h"
#include "primebds/utils/database/user_db.h"
#include "primebds/utils/database/session_db.h"
#include "primebds/utils/database/server_db.h"
#include "primebds/utils/permissions/permission_manager.h"
#include "primebds/utils/logging.h"

#include <ctime>
#include <filesystem>

namespace primebds
{

    // ---------------------------------------------------------------------------
    // Plugin lifecycle
    // ---------------------------------------------------------------------------

    void PrimeBDS::onLoad()
    {
        getLogger().info("PrimeBDS v{} loading...", getDescription().getVersion());

        // Ensure data directory exists
        std::filesystem::create_directories(getDataFolder());

        // Point ConfigManager at Endstone's data folder
        config::ConfigManager::setDataFolder(getDataFolder().string());

        // Initialise databases
        db = std::make_unique<db::UserDB>((getDataFolder() / "users.db").string());
        sldb = std::make_unique<db::SessionDB>((getDataFolder() / "sessions.db").string());
        serverdb = std::make_unique<db::ServerDB>((getDataFolder() / "server.db").string());

        // Load configuration
        config::ConfigManager::instance().load();

        last_shutdown_time = (int64_t)std::time(nullptr);
    }

    void PrimeBDS::onEnable()
    {
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

    void PrimeBDS::onDisable()
    {
        getLogger().info("PrimeBDS v{} disabled.", getDescription().getVersion());

        // End all active sessions
        for (auto *player : getServer().getOnlinePlayers())
        {
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
                             const std::vector<std::string> &args)
    {
        auto &registry = CommandRegistry::instance();
        auto *reg = registry.find(command.getName());
        if (reg)
        {
            if (!config::ConfigManager::instance().isCommandEnabled(command.getName()))
            {
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

    void PrimeBDS::reloadCustomPerms(endstone::Player &player)
    {
        auto &pm = permissions::PermissionManager::instance();
        auto user = db->getOnlineUser(player.getXuid());
        std::string rank = (user && !user->internal_rank.empty()) ? user->internal_rank : "default";
        auto perms = pm.getRankPermissions(rank);
        for (auto &[perm, value] : perms)
        {
            // Apply rank permissions to the player
            // endstone doesn't have per-player permission attachment yet,
            // so this is a placeholder for future API support
        }
    }

    void PrimeBDS::checkForInactiveSessions()
    {
        // End any sessions that are still open (from unclean shutdown)
        auto active = sldb->getActiveSessions();
        for (auto &session : active)
        {
            sldb->endSession(session["xuid"]);
        }
    }

    // ---------------------------------------------------------------------------
    // EventListener delegation
    // ---------------------------------------------------------------------------

    void EventListener::onPlayerDeath(endstone::PlayerDeathEvent &event)
    {
        handlers::handleDeathEvent(plugin_, event);
    }

    void EventListener::onPlayerTeleport(endstone::PlayerTeleportEvent &event)
    {
        handlers::handleTeleportEvent(plugin_, event);
    }

    void EventListener::onPlayerBedEnter(endstone::PlayerBedEnterEvent &event)
    {
        handlers::handleBedEnterEvent(plugin_, event);
    }

    void EventListener::onPlayerEmote(endstone::PlayerEmoteEvent &event)
    {
        handlers::handleEmoteEvent(plugin_, event);
    }

    void EventListener::onPlayerSkinChange(endstone::PlayerSkinChangeEvent &event)
    {
        handlers::handleSkinChangeEvent(plugin_, event);
    }

    void EventListener::onLeavesDecay(endstone::LeavesDecayEvent &event)
    {
        handlers::handleLeavesDecayEvent(plugin_, event);
    }

    void EventListener::onPlayerGameModeChange(endstone::PlayerGameModeChangeEvent &event)
    {
        handlers::handleGamemodeEvent(plugin_, event);
    }

    void EventListener::onPlayerInteractActor(endstone::PlayerInteractActorEvent &event)
    {
        handlers::handleInteractEvent(plugin_, event);
    }

    void EventListener::onItemPickup(endstone::PlayerPickupItemEvent &event)
    {
        handlers::handleItemPickupEvent(plugin_, event);
    }

    void EventListener::onEntityDamage(endstone::ActorDamageEvent &event)
    {
        handlers::combat::handleDamageEvent(plugin_, event);
    }

    void EventListener::onEntityKnockback(endstone::ActorKnockbackEvent &event)
    {
        handlers::combat::handleKnockbackEvent(plugin_, event);
    }

    void EventListener::onPlayerLogin(endstone::PlayerLoginEvent &event)
    {
        handlers::connections::handleLoginEvent(plugin_, event);
    }

    void EventListener::onPlayerJoin(endstone::PlayerJoinEvent &event)
    {
        handlers::connections::handleJoinEvent(plugin_, event);
    }

    void EventListener::onPlayerQuit(endstone::PlayerQuitEvent &event)
    {
        handlers::connections::handleLeaveEvent(plugin_, event);
    }

    void EventListener::onPlayerKick(endstone::PlayerKickEvent &event)
    {
        handlers::connections::handleKickEvent(plugin_, event);
    }

    void EventListener::onPlayerCommand(endstone::PlayerCommandEvent &event)
    {
        handlers::preprocesses::handleCommandPreprocess(plugin_, event);
    }

    void EventListener::onServerCommand(endstone::ServerCommandEvent &event)
    {
        handlers::preprocesses::handleServerCommandPreprocess(plugin_, event);
    }

    void EventListener::onPlayerChat(endstone::PlayerChatEvent &event)
    {
        handlers::handleChatEvent(plugin_, event);
    }

    void EventListener::onServerLoad(endstone::ServerLoadEvent &event)
    {
        handlers::handleServerLoadEvent(plugin_, event);
    }

} // namespace primebds

// ---------------------------------------------------------------------------
// Endstone plugin entry point
// ---------------------------------------------------------------------------

ENDSTONE_PLUGIN("primebds", "3.4.0", primebds::PrimeBDS)
{
    description = "An essentials plugin for diagnostics, stability, and quality of life on Minecraft Bedrock Edition.";
    authors = {"primebds"};

    // All commands are declared here for endstone registration.
    // Dispatch is handled by PrimeBDS::onCommand via CommandRegistry.
    command("activity");
    command("activitylist");
    command("afk");
    command("alist");
    command("alts");
    command("altspy");
    command("back");
    command("blockinfo");
    command("blockscan");
    command("bossbar");
    command("bottom");
    command("broadcast");
    command("check");
    command("clearchat");
    command("cords");
    command("discord");
    command("enchantforce");
    command("entityinfo");
    command("feed");
    command("filterlist");
    command("fly");
    command("giveforce");
    command("globalmute");
    command("gma");
    command("gmc");
    command("gms");
    command("gmsp");
    command("gmt");
    command("god");
    command("hat");
    command("heal");
    command("home");
    command("homeother");
    command("ipban");
    command("ipmute");
    command("iteminfo");
    command("itemlore");
    command("itemname");
    command("itemtag");
    command("modspy");
    command("monitor");
    command("more");
    command("motd");
    command("msgtoggle");
    command("mute");
    command("nameban");
    command("nameunban");
    command("nickname");
    command("note");
    command("offlinetp");
    command("permban");
    command("permissions");
    command("permissionslist");
    command("ping");
    command("playtime");
    command("popup");
    command("primebds");
    command("punishments");
    command("rank");
    command("reloadscripts");
    command("removeban");
    command("repair");
    command("reply");
    command("rules");
    command("send");
    command("setback");
    command("sethomes");
    command("setrules");
    command("setspawn");
    command("silentmute");
    command("socialspy");
    command("spawn");
    command("spectate");
    command("speed");
    command("staffchat");
    command("tempban");
    command("tempmute");
    command("tip");
    command("toast");
    command("top");
    command("unmute");
    command("unwarn");
    command("updatepacks");
    command("voice");
    command("warn");
    command("warnings");
    command("warp");
    command("warps");
}
