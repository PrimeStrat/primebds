#include "primebds/commands/command_metadata.h"

namespace primebds {

void registerEndstoneCommands(endstone::detail::PluginDescriptionBuilder &b) {

    // -----------------------------------------------------------------------
    // GAMEMODE
    // -----------------------------------------------------------------------
    b.command("gma").description("Sets your game mode to adventure!")
        .usages("/gma [player: player]")
        .permissions("primebds.command.gma");

    b.command("gmc").description("Sets your game mode to creative!")
        .usages("/gmc [player: player]")
        .permissions("primebds.command.gmc");

    b.command("gms").description("Sets your game mode to survival!")
        .usages("/gms [player: player]")
        .permissions("primebds.command.gms");

    b.command("gmsp").description("Sets your game mode to spectator!")
        .usages("/gmsp [player: player]")
        .permissions("primebds.command.gmsp");

    b.command("gmt").description("Toggles you between survival and creative mode!")
        .usages("/gmt [player: player]")
        .permissions("primebds.command.gmt");

    // -----------------------------------------------------------------------
    // ITEM
    // -----------------------------------------------------------------------
    b.command("enchantforce").description("Forces a given enchantment onto an item!")
        .usages("/enchantforce <player: player> [enchantment: string] [level: int]")
        .permissions("primebds.command.enchantforce")
        .aliases("enchantf", "forceenchant");

    b.command("giveforce").description("Forces any item registered to be given, hidden or not!")
        .usages("/giveforce <player: player> <item: string> [amount: int] [data: int]")
        .permissions("primebds.command.giveforce")
        .aliases("givef", "forcegive");

    b.command("hat").description("Sets the item in-hand as a hat!")
        .usages("/hat [player: player]")
        .permissions("primebds.command.hat", "primebds.command.hat.other");

    b.command("iteminfo").description("Check item data!")
        .usages("/iteminfo")
        .permissions("primebds.command.iteminfo");

    b.command("itemlore").description("Modify item lore data!")
        .usages("/itemlore <player: player> (add|set|delete|clear)<action: lore_action> [text: string]")
        .permissions("primebds.command.itemlore")
        .aliases("lore");

    b.command("itemname").description("Modify item name data!")
        .usages("/itemname <player: player> (set|clear)<action: name_action> [name: string]")
        .permissions("primebds.command.itemname");

    b.command("itemtag").description("Modify item tags!")
        .usages("/itemtag <player: player> (unbreakable)<tag: item_tag> <value: bool>")
        .permissions("primebds.command.itemtag");

    b.command("more").description("Sets a full stack to the held item!")
        .usages("/more")
        .permissions("primebds.command.more");

    b.command("repair").description("Repairs the item in hand!")
        .usages("/repair [player: player]")
        .permissions("primebds.command.repair", "primebds.command.repair.other");

    // -----------------------------------------------------------------------
    // JAVA PARODY
    // -----------------------------------------------------------------------
    b.command("alist").description("Manages server allowlist profiles and server allowlist!")
        .usages(
            "/alist (list|check|profiles)<action: alist_action> [args: message]",
            "/alist (add|remove)<action: alist_action> <player: string> [ignore_max_player_limit: bool]",
            "/alist (create|use|delete|clear)<action: alist_action> <name: string>",
            "/alist (inherit)<action: alist_action> <child: string> <parent: string>")
        .permissions("primebds.command.alist")
        .aliases("wlist");

    b.command("bossbar").description("Sets or clears a client-sided bossbar display!")
        .usages(
            "/bossbar <player: player> (red|blue|green|yellow|pink|purple|rebecca_purple|white)<color: bar_color> <percent: float> <title: message>",
            "/bossbar <player: player> (clear)<action: bar_action>")
        .permissions("primebds.command.bossbar");

    b.command("spectate").description("Warp to a player to spectate them!")
        .usages("/spectate [player: player]")
        .permissions("primebds.command.spectate");

    // -----------------------------------------------------------------------
    // MESSAGE
    // -----------------------------------------------------------------------
    b.command("broadcast").description("Broadcasts a message to the entire server!")
        .usages("/broadcast <message: message>")
        .permissions("primebds.command.broadcast")
        .aliases("bc");

    b.command("clearchat").description("Clears the chat for all players!")
        .usages("/clearchat")
        .permissions("primebds.command.clearchat")
        .aliases("cc");

    b.command("discord").description("Shows the Discord invite link!")
        .usages("/discord")
        .permissions("primebds.command.discord");

    b.command("motd").description("Displays or sets the Message of the Day!")
        .usages("/motd [message: message]")
        .permissions("primebds.command.motd");

    b.command("msgtoggle").description("Toggles private messages on or off!")
        .usages("/msgtoggle")
        .permissions("primebds.command.msgtoggle");

    b.command("note").description("Add, remove, clear, or list notes on a player!")
        .usages(
            "/note <player: player> (add)<action: note_action> <text: message>",
            "/note <player: player> (remove)<action: note_action> <id: int>",
            "/note <player: player> (clear)<action: note_action> [args: message]",
            "/note <player: player> (list)<action: note_action> [page: int]")
        .permissions("primebds.command.note");

    b.command("popup").description("Sends a popup message to a player!")
        .usages("/popup <player: player> <message: message>")
        .permissions("primebds.command.popup");

    b.command("reply").description("Reply to the last person who messaged you!")
        .usages("/reply <message: message>")
        .permissions("primebds.command.reply")
        .aliases("r");

    b.command("rules").description("Displays the server rules!")
        .usages("/rules")
        .permissions("primebds.command.rules");

    b.command("setrules").description("Manages the server rules list!")
        .usages(
            "/setrules (add)<action: rules_action> <text: message>",
            "/setrules (edit)<action: rules_action> <index: int> <text: message>",
            "/setrules (delete)<action: rules_action> <index: int>",
            "/setrules (insert)<action: rules_action> <index: int> <text: message>",
            "/setrules (list)<action: rules_action> [page: int]")
        .permissions("primebds.command.setrules");

    b.command("socialspy").description("Toggle social spy to see private messages!")
        .usages("/socialspy")
        .permissions("primebds.command.socialspy");

    b.command("staffchat").description("Toggle staff chat or send a staff-only message!")
        .usages("/staffchat [message: message]")
        .permissions("primebds.command.staffchat")
        .aliases("sc");

    b.command("tip").description("Sends a tip message to a player!")
        .usages("/tip <player: player> <message: message>")
        .permissions("primebds.command.tip");

    b.command("toast").description("Sends a toast notification to a player!")
        .usages("/toast <player: player> <title: string> <message: message>")
        .permissions("primebds.command.toast");

    b.command("voice").description("Enables voice chat attachment!")
        .usages("/voice")
        .permissions("primebds.command.voice");

    // -----------------------------------------------------------------------
    // MISC
    // -----------------------------------------------------------------------
    b.command("activity").description("Lists out session information!")
        .usages("/activity <player: player> [page: int]")
        .permissions("primebds.command.activity");

    b.command("activitylist").description("Lists players by activity filter!")
        .usages("/activitylist (highest|lowest|recent)<filter: activity_filter> [page: int]")
        .permissions("primebds.command.activitylist");

    b.command("afk").description("Toggles AFK mode for yourself!")
        .usages("/afk")
        .permissions("primebds.command.afk");

    b.command("blockinfo").description("Prints info of the facing block!")
        .usages("/blockinfo")
        .permissions("primebds.command.blockinfo");

    b.command("blockscan").description("Continuously show information about the block you're looking at.")
        .usages("/blockscan [enable: bool]")
        .permissions("primebds.command.blockscan");

    b.command("check").description("Checks a player's client info!")
        .usages("/check <player: player> (info|mod|network|world)[info: info]")
        .permissions("primebds.command.check")
        .aliases("seen");

    b.command("cords").description("Print your current position!")
        .usages("/cords")
        .permissions("primebds.command.cords")
        .aliases("blockpos", "pos");

    b.command("entityinfo").description("Check entity information!")
        .usages("/entityinfo (list)<action: entity_action> [page: int]")
        .permissions("primebds.command.entityinfo");

    b.command("feed").description("Sets player hunger to full!")
        .usages("/feed [player: player]")
        .permissions("primebds.command.feed", "primebds.command.feed.other")
        .aliases("eat");

    b.command("god").description("Toggles invulnerability!")
        .usages("/god [player: player] [toggle: bool]")
        .permissions("primebds.command.god", "primebds.command.god.other");

    b.command("heal").description("Heals all health to full!")
        .usages("/heal [player: player]")
        .permissions("primebds.command.heal", "primebds.command.heal.other");

    b.command("nickname").description("Set your display name!")
        .usages("/nickname [name: string]")
        .permissions("primebds.command.nickname")
        .aliases("nick");

    b.command("ping").description("Check your or another player's latency!")
        .usages("/ping [player: player]")
        .permissions("primebds.command.ping");

    b.command("playtime").description("Check total playtime!")
        .usages("/playtime [player: player]")
        .permissions("primebds.command.playtime");

    // -----------------------------------------------------------------------
    // MODERATION
    // -----------------------------------------------------------------------
    b.command("alts").description("Check for alternate accounts!")
        .usages("/alts <player: player>")
        .permissions("primebds.command.alts");

    b.command("altspy").description("Toggle alt detection notifications!")
        .usages("/altspy")
        .permissions("primebds.command.altspy");

    b.command("globalmute").description("Toggles global mute for the server!")
        .usages("/globalmute")
        .permissions("primebds.command.globalmute")
        .aliases("gmute");

    b.command("ipban").description("IP bans a player from the server!")
        .usages("/ipban <player: player> <duration: int> <unit: string> [reason: message]")
        .permissions("primebds.command.ipban");

    b.command("ipmute").description("IP mutes a player on the server!")
        .usages("/ipmute <player: player> <duration: int> <unit: string> [reason: message]")
        .permissions("primebds.command.ipmute");

    b.command("modspy").description("Toggle moderation spy notifications!")
        .usages("/modspy")
        .permissions("primebds.command.modspy");

    b.command("mute").description("Permanently mutes a player on the server!")
        .usages("/mute <player: player> [reason: message]")
        .permissions("primebds.command.mute");

    b.command("nameban").description("Bans a player name from the server!")
        .usages("/nameban <name: string> <duration: int> <unit: string> [reason: message]")
        .permissions("primebds.command.nameban");

    b.command("nameunban").description("Unbans a player name from the server!")
        .usages("/nameunban <name: string>")
        .permissions("primebds.command.nameunban");

    b.command("permban").description("Permanently bans a player from the server!")
        .usages("/permban <player: player> [reason: message]")
        .permissions("primebds.command.permban");

    b.command("punishments").description("View a player's punishment history!")
        .usages("/punishments <player: player> [page: int]")
        .permissions("primebds.command.punishments");

    b.command("removeban").description("Removes a ban from a player!")
        .usages("/removeban <player: player>")
        .permissions("primebds.command.removeban")
        .aliases("unban");

    b.command("silentmute").description("Silently mutes a player (messages appear only to them)!")
        .usages("/silentmute <player: player>")
        .permissions("primebds.command.silentmute")
        .aliases("smute");

    b.command("tempban").description("Temporarily bans a player from the server!")
        .usages("/tempban <player: player> <duration: int> <unit: string> [reason: message]")
        .permissions("primebds.command.tempban");

    b.command("tempmute").description("Temporarily mutes a player on the server!")
        .usages("/tempmute <player: player> <duration: int> <unit: string> [reason: message]")
        .permissions("primebds.command.tempmute");

    b.command("unmute").description("Removes an active mute from a player!")
        .usages("/unmute <player: player>")
        .permissions("primebds.command.unmute");

    b.command("unwarn").description("Remove a warning or clear all warnings from a player!")
        .usages(
            "/unwarn <player: player> (clear)<action: unwarn_action> [args: message]",
            "/unwarn <player: player> [id: int]")
        .permissions("primebds.command.unwarn");

    b.command("warn").description("Warn a player that they are breaking a rule!")
        .usages("/warn <player: player> <reason: string> [duration: int] [unit: string]")
        .permissions("primebds.command.warn");

    b.command("warnings").description("List or delete warnings for a player!")
        .usages(
            "/warnings <player: player> [page: int]",
            "/warnings <player: player> (delete|clear)<action: warn_action> <id: int>")
        .permissions("primebds.command.warnings");

    // -----------------------------------------------------------------------
    // MOVEMENT
    // -----------------------------------------------------------------------
    b.command("back").description("Teleport to your last saved location!")
        .usages("/back")
        .permissions("primebds.command.back");

    b.command("bottom").description("Teleport to the lowest safe position!")
        .usages("/bottom")
        .permissions("primebds.command.bottom");

    b.command("fly").description("Toggles flight for a player!")
        .usages("/fly [player: player]")
        .permissions("primebds.command.fly");

    b.command("home").description("Manage and warp to homes!")
        .usages(
            "/home",
            "/home (set)<action: home_action> [name: string]",
            "/home (list)<action: home_action> [page: int]",
            "/home (warp)<action: home_action> <name: string>",
            "/home (del|delete)<action: home_action> <name: string>",
            "/home (max)<action: home_action> [limit: int]")
        .permissions("primebds.command.home");

    b.command("homeother").description("Manage another player's homes!")
        .usages(
            "/homeother <player: player>",
            "/homeother <player: player> (list)<action: homeother_action> [page: int]",
            "/homeother <player: player> (warp)<action: homeother_action> <name: string>",
            "/homeother <player: player> (set)<action: homeother_action> [name: string]",
            "/homeother <player: player> (del)<action: homeother_action> <name: string>",
            "/homeother <player: player> (max)<action: homeother_action> [limit: int]")
        .permissions("primebds.command.homeother");

    b.command("offlinetp").description("Teleport to where a player last logged out.")
        .usages("/offlinetp <player: player>")
        .permissions("primebds.command.offlinetp")
        .aliases("otp");

    b.command("setback").description("Sets the global cooldown and delay for /back!")
        .usages("/setback [delay: int] [cooldown: int]")
        .permissions("primebds.command.setback");

    b.command("sethomes").description("Set global home settings (delay, cooldown, cost)!")
        .usages("/sethomes <delay: int> <cooldown: int> <cost: int>")
        .permissions("primebds.command.sethomes");

    b.command("setspawn").description("Sets the global spawn point!")
        .usages("/setspawn")
        .permissions("primebds.command.setspawn");

    b.command("spawn").description("Warps you to the spawn!")
        .usages("/spawn")
        .permissions("primebds.command.spawn");

    b.command("speed").description("Modifies player flyspeed or walkspeed!")
        .usages(
            "/speed <value: float>",
            "/speed (flyspeed|walkspeed)<mode: speed_mode> <value: float> [player: player]",
            "/speed (reset)<action: speed_action> <type: string> [player: player]")
        .permissions("primebds.command.speed");

    b.command("top").description("Warps you to the topmost block with air!")
        .usages("/top")
        .permissions("primebds.command.top");

    b.command("warp").description("Warp to a warp location!")
        .usages(
            "/warp",
            "/warp <name: message>",
            "/warp (list)<action: warp_action> [page: int]")
        .permissions("primebds.command.warp");

    b.command("warps").description("Manage server warps!")
        .usages(
            "/warps",
            "/warps (list)<action: warps_action> [page: int]",
            "/warps (create)<action: warps_action> <name: string> [displayname: string] [category: string] [description: string]",
            "/warps (delete)<action: warps_action> <name: message>",
            "/warps (addalias)<action: warps_action> <name: string> <alias: message>",
            "/warps (removealias)<action: warps_action> <name: string> <alias: message>")
        .permissions("primebds.command.warps");

    // -----------------------------------------------------------------------
    // SERVER
    // -----------------------------------------------------------------------
    b.command("filterlist").description("Lists all players with a filter!")
        .usages("/filterlist (ops|default|online|offline|muted|banned|ipbanned)<plist_filter: plist_filter> [page: int]")
        .permissions("primebds.command.filterlist")
        .aliases("flist");

    b.command("monitor").description("Monitor server performance in real time!")
        .usages("/monitor [enable: bool]")
        .permissions("primebds.command.monitor");

    b.command("permissions").description("Sets the internal permissions for a player!")
        .usages("/permissions <player: player> (settrue|setfalse|setneutral)<set_perm: set_perm> <permission: message>")
        .permissions("primebds.command.permissions")
        .aliases("perms");

    b.command("permissionslist").description("View the global or player-specific permissions list!")
        .usages(
            "/permissionslist",
            "/permissionslist <player: player>")
        .permissions("primebds.command.permissionslist")
        .aliases("permslist");

    b.command("primebds").description("PrimeBDS management command!")
        .usages(
            "/primebds",
            "/primebds (info|reloadconfig)<action: pbds_action> [args: message]",
            "/primebds (config)<action: pbds_action> <keypath: string> [value: message]")
        .permissions("primebds.command.primebds");

    b.command("rank").description("Manage server ranks!")
        .usages(
            "/rank (set)<sub: rank_sub> <player: player> <rank: string>",
            "/rank (create)<sub: rank_sub> <name: string>",
            "/rank (delete)<sub: rank_sub> <name: string>",
            "/rank (info)<sub: rank_sub> <name: string>",
            "/rank (perm)<sub: rank_sub> <action: string> <rank: string> <permission: message>",
            "/rank (list)<sub: rank_sub> [page: int]",
            "/rank (inherit)<sub: rank_sub> <rank: string> <parent: string>",
            "/rank (weight)<sub: rank_sub> <rank: string> <weight: int>",
            "/rank (prefix)<sub: rank_sub> <rank: string> <prefix: message>",
            "/rank (suffix)<sub: rank_sub> <rank: string> <suffix: message>")
        .permissions("primebds.command.rank");

    b.command("reloadscripts").description("Reloads server scripts!")
        .usages("/reloadscripts")
        .permissions("primebds.command.reloadscripts")
        .aliases("rscripts", "rs");

    b.command("send").description("Send players to another server!")
        .usages("/send <player: player> <address: message>")
        .permissions("primebds.command.send");

    b.command("updatepacks").description("Update resource or behavior pack versions!")
        .usages("/updatepacks (resource|behavior)<type: pack_type> [version: string]")
        .permissions("primebds.command.updatepacks");
}

} // namespace primebds
