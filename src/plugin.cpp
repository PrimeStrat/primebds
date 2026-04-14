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

    // --- GAMEMODE ---
    command("gma").description("Sets your game mode to adventure!").usages("/gma [player: player]").permissions("primebds.command.gma");
    command("gmc").description("Sets your game mode to creative!").usages("/gmc [player: player]").permissions("primebds.command.gmc");
    command("gms").description("Sets your game mode to survival!").usages("/gms [player: player]").permissions("primebds.command.gms");
    command("gmsp").description("Sets your game mode to spectator!").usages("/gmsp [player: player]").permissions("primebds.command.gmsp");
    command("gmt").description("Toggles you between survival and creative mode!").usages("/gmt [player: player]").permissions("primebds.command.gmt");

    // --- ITEM ---
    command("enchantforce").description("Forces a given enchantment onto an item!").usages("/enchantforce <player: player> [enchantment: string] [level: int]").permissions("primebds.command.enchantforce").aliases("enchantf", "forceenchant");
    command("giveforce").description("Forces any item registered to be given, hidden or not!").usages("/giveforce <player: player> <item: string> [amount: int] [data: int]").permissions("primebds.command.giveforce").aliases("givef", "forcegive");
    command("hat").description("Sets the item in-hand as a hat!").usages("/hat [player: player]").permissions("primebds.command.hat", "primebds.command.hat.other");
    command("iteminfo").description("Check item data!").usages("/iteminfo").permissions("primebds.command.iteminfo");
    command("itemlore").description("Modify item lore data!").usages("/itemlore <player: player> (add|set|delete|clear) [text: string]").permissions("primebds.command.itemlore").aliases("lore");
    command("itemname").description("Modify item name data!").usages("/itemname <player: player> (set|clear) [name: string]").permissions("primebds.command.itemname");
    command("itemtag").description("Modify item tags!").usages("/itemtag <player: player> (unbreakable) <value: bool>").permissions("primebds.command.itemtag");
    command("more").description("Sets a full stack to the held item!").usages("/more").permissions("primebds.command.more");
    command("repair").description("Repairs the item in hand!").usages("/repair [player: player]").permissions("primebds.command.repair", "primebds.command.repair.other");

    // --- JAVA PARODY ---
    command("alist").description("Manage the allowlist profiles!").usages("/alist (list)", "/alist (add) <player: string>", "/alist (remove) <player: string>", "/alist (check) <player: string>", "/alist (profiles)", "/alist (create) <profile: string>", "/alist (delete) <profile: string>", "/alist (use) <profile: string>", "/alist (inherit) <profile: string> <parent: string>", "/alist (clear)").permissions("primebds.command.alist");
    command("bossbar").description("Sets or clears a client-sided bossbar display!").usages("/bossbar <player: player> (red|blue|green|yellow|pink|purple|rebecca_purple|white) <percent: float> <title: message>", "/bossbar <player: player> (clear)").permissions("primebds.command.bossbar");
    command("spectate").description("Warp to a player to spectate them!").usages("/spectate [player: player]").permissions("primebds.command.spectate");

    // --- MESSAGE ---
    command("broadcast").description("Broadcasts a message to the entire server!").usages("/broadcast <message: message>").permissions("primebds.command.broadcast").aliases("bc");
    command("clearchat").description("Clears the chat for all players!").usages("/clearchat").permissions("primebds.command.clearchat").aliases("cc");
    command("discord").description("Shows the Discord invite link!").usages("/discord").permissions("primebds.command.discord");
    command("motd").description("Displays or sets the Message of the Day!").usages("/motd [message: message]").permissions("primebds.command.motd");
    command("msgtoggle").description("Toggles private messages on or off!").usages("/msgtoggle").permissions("primebds.command.msgtoggle");
    command("note").description("Add, remove, clear, or list notes on a player!").usages("/note <player: player> (add) <text: message>", "/note <player: player> (remove) <id: int>", "/note <player: player> (clear)", "/note <player: player> (list) [page: int]").permissions("primebds.command.note");
    command("popup").description("Sends a popup message to a player!").usages("/popup <player: player> <message: message>").permissions("primebds.command.popup");
    command("reply").description("Reply to the last person who messaged you!").usages("/reply <message: message>").permissions("primebds.command.reply").aliases("r");
    command("rules").description("Displays the server rules!").usages("/rules").permissions("primebds.command.rules");
    command("setrules").description("Manages the server rules list!").usages("/setrules (add) <text: message>", "/setrules (edit) <index: int> <text: message>", "/setrules (delete) <index: int>", "/setrules (insert) <index: int> <text: message>", "/setrules (list)").permissions("primebds.command.setrules");
    command("socialspy").description("Toggle social spy to see private messages!").usages("/socialspy").permissions("primebds.command.socialspy");
    command("staffchat").description("Toggle staff chat or send a staff-only message!").usages("/staffchat [message: message]").permissions("primebds.command.staffchat").aliases("sc");
    command("tip").description("Sends a tip message to a player!").usages("/tip <player: player> <message: message>").permissions("primebds.command.tip");
    command("toast").description("Sends a toast notification to a player!").usages("/toast <player: player> <title: string> <message: message>").permissions("primebds.command.toast");
    command("voice").description("Enables voice chat attachment!").usages("/voice").permissions("primebds.command.voice");

    // --- MISC ---
    command("activity").description("Lists out session information!").usages("/activity <player: player> [page: int]").permissions("primebds.command.activity");
    command("activitylist").description("Lists players by activity filter!").usages("/activitylist [page: int] (highest|lowest|recent)").permissions("primebds.command.activitylist");
    command("afk").description("Toggles AFK mode for yourself!").usages("/afk").permissions("primebds.command.afk");
    command("blockinfo").description("Prints info of the facing block!").usages("/blockinfo").permissions("primebds.command.blockinfo");
    command("blockscan").description("Continuously show information about the block you're looking at.").usages("/blockscan (disable)").permissions("primebds.command.blockscan");
    command("check").description("Checks a player's client info!").usages("/check <player: player> (info|mod|network|world)[info: info]").permissions("primebds.command.check").aliases("seen");
    command("cords").description("Print your current position!").usages("/cords").permissions("primebds.command.cords").aliases("blockpos", "pos");
    command("entityinfo").description("Check entity information!").usages("/entityinfo (list) [page: int]").permissions("primebds.command.entityinfo");
    command("feed").description("Sets player hunger to full!").usages("/feed [player: player]").permissions("primebds.command.feed", "primebds.command.feed.other").aliases("eat");
    command("god").description("Toggles invulnerability!").usages("/god [player: player] [toggle: bool]").permissions("primebds.command.god", "primebds.command.god.other");
    command("heal").description("Heals all health to full!").usages("/heal [player: player]").permissions("primebds.command.heal", "primebds.command.heal.other");
    command("nickname").description("Set your display name!").usages("/nickname [name: string]").permissions("primebds.command.nickname").aliases("nick");
    command("ping").description("Check your or another player's latency!").usages("/ping [player: player]").permissions("primebds.command.ping");
    command("playtime").description("Check total playtime!").usages("/playtime [player: player]").permissions("primebds.command.playtime");

    // --- MODERATION ---
    command("alts").description("Check for alternate accounts!").usages("/alts <player: player>").permissions("primebds.command.alts");
    command("altspy").description("Toggle alt detection notifications!").usages("/altspy").permissions("primebds.command.altspy");
    command("globalmute").description("Toggles global mute for the server!").usages("/globalmute").permissions("primebds.command.globalmute").aliases("gmute");
    command("ipban").description("IP bans a player from the server!").usages("/ipban <player: player> <duration: int> <unit: string> [reason: message]").permissions("primebds.command.ipban");
    command("ipmute").description("IP mutes a player on the server!").usages("/ipmute <player: player> <duration: int> <unit: string> [reason: message]").permissions("primebds.command.ipmute");
    command("modspy").description("Toggle moderation spy notifications!").usages("/modspy").permissions("primebds.command.modspy");
    command("mute").description("Permanently mutes a player on the server!").usages("/mute <player: player> [reason: message]").permissions("primebds.command.mute");
    command("nameban").description("Bans a player name from the server!").usages("/nameban <name: string> <duration: int> <unit: string> [reason: message]").permissions("primebds.command.nameban");
    command("nameunban").description("Unbans a player name from the server!").usages("/nameunban <name: string>").permissions("primebds.command.nameunban");
    command("permban").description("Permanently bans a player from the server!").usages("/permban <player: player> [reason: message]").permissions("primebds.command.permban");
    command("punishments").description("View a player's punishment history!").usages("/punishments <player: player> [page: int]").permissions("primebds.command.punishments");
    command("removeban").description("Removes a ban from a player!").usages("/removeban <player: player>").permissions("primebds.command.removeban").aliases("unban");
    command("silentmute").description("Silently mutes a player (messages appear only to them)!").usages("/silentmute <player: player>").permissions("primebds.command.silentmute").aliases("smute");
    command("tempban").description("Temporarily bans a player from the server!").usages("/tempban <player: player> <duration: int> <unit: string> [reason: message]").permissions("primebds.command.tempban");
    command("tempmute").description("Temporarily mutes a player on the server!").usages("/tempmute <player: player> <duration: int> <unit: string> [reason: message]").permissions("primebds.command.tempmute");
    command("unmute").description("Removes an active mute from a player!").usages("/unmute <player: player>").permissions("primebds.command.unmute");
    command("unwarn").description("Remove a warning or clear all warnings from a player!").usages("/unwarn <player: player> (clear)", "/unwarn <player: player> [id: int]").permissions("primebds.command.unwarn");
    command("warn").description("Warn a player that they are breaking a rule!").usages("/warn <player: player> <reason: string> [duration: int] [unit: string]").permissions("primebds.command.warn");
    command("warnings").description("List or delete warnings for a player!").usages("/warnings <player: player> [page: int]", "/warnings <player: player> (delete|clear) [id: int]").permissions("primebds.command.warnings");

    // --- MOVEMENT ---
    command("back").description("Teleport to your last saved location!").usages("/back").permissions("primebds.command.back");
    command("bottom").description("Teleport to the lowest safe position!").usages("/bottom").permissions("primebds.command.bottom");
    command("fly").description("Toggles flight for a player!").usages("/fly [player: player]").permissions("primebds.command.fly");
    command("home").description("Manage your homes!").usages("/home", "/home (set) [name: string]", "/home (list)", "/home (warp) <name: string>", "/home (del|delete) <name: string>", "/home (max)").permissions("primebds.command.home");
    command("homeother").description("Manage another player's homes!").usages("/homeother <player: player>", "/homeother <player: player> (list)", "/homeother <player: player> (warp) <name: string>", "/homeother <player: player> (set) [name: string]", "/homeother <player: player> (del) <name: string>", "/homeother <player: player> (max)").permissions("primebds.command.homeother");
    command("offlinetp").description("Teleport to where a player last logged out.").usages("/offlinetp <player: player>").permissions("primebds.command.offlinetp").aliases("otp");
    command("setback").description("Sets the global cooldown and delay for /back!").usages("/setback [delay: int] [cooldown: int]").permissions("primebds.command.setback");
    command("sethomes").description("Set global home settings (delay, cooldown, cost)!").usages("/sethomes <delay: int> <cooldown: int> <cost: int>").permissions("primebds.command.sethomes");
    command("setspawn").description("Sets the global spawn point!").usages("/setspawn").permissions("primebds.command.setspawn");
    command("spawn").description("Warps you to the spawn!").usages("/spawn").permissions("primebds.command.spawn");
    command("speed").description("Modifies player flyspeed or walkspeed!").usages("/speed <value: float>", "/speed (flyspeed|walkspeed) <value: float> [player: player]", "/speed (reset) (flyspeed|walkspeed) [player: player]").permissions("primebds.command.speed");
    command("top").description("Warps you to the topmost block with air!").usages("/top").permissions("primebds.command.top");
    command("warp").description("Warp to a warp location!").usages("/warp", "/warp <name: message>", "/warp (list)").permissions("primebds.command.warp");
    command("warps").description("Manage server warps!").usages("/warps", "/warps (list)", "/warps (create) <name: string> [displayname: string] [category: string] [description: string]", "/warps (delete) <name: message>", "/warps (addalias) <name: string> <alias: message>", "/warps (removealias) <name: string> <alias: message>").permissions("primebds.command.warps");

    // --- SERVER ---
    command("filterlist").description("List players by filter!").usages("/filterlist <ops|default|online|offline|muted|banned|ipbanned> [page: int]").permissions("primebds.command.filterlist").aliases("flist");
    command("monitor").description("Monitor server performance in real time!").usages("/monitor (server|disable)").permissions("primebds.command.monitor");
    command("permissions").description("Set internal permissions for a player!").usages("/permissions <settrue|setfalse|setneutral> <player: player> <permission: message>").permissions("primebds.command.permissions").aliases("perms");
    command("permissionslist").description("View custom permissions!").usages("/permissionslist", "/permissionslist <player: player>").permissions("primebds.command.permissionslist").aliases("permslist");
    command("primebds").description("PrimeBDS management command!").usages("/primebds", "/primebds (info)", "/primebds (reloadconfig)", "/primebds (config) <key.path: message> [value: message]").permissions("primebds.command.primebds");
    command("rank").description("Manage server ranks!").usages("/rank (set) <player: player> <rank: string>", "/rank (create) <name: string>", "/rank (delete) <name: string>", "/rank (info) <name: string>", "/rank (perm) <add|remove> <rank: string> <permission: message>", "/rank (list)", "/rank (inherit) <rank: string> <parent: string>", "/rank (weight) <rank: string> <weight: int>", "/rank (prefix) <rank: string> <prefix: message>", "/rank (suffix) <rank: string> <suffix: message>").permissions("primebds.command.rank");
    command("reloadscripts").description("Reloads server scripts!").usages("/reloadscripts").permissions("primebds.command.reloadscripts").aliases("rscripts", "rs");
    command("send").description("Send players to another server!").usages("/send <player: player> <ip:port: message>").permissions("primebds.command.send");
    command("updatepacks").description("Update resource or behavior pack versions!").usages("/updatepacks <resource|behavior> [version: string]").permissions("primebds.command.updatepacks");
}
