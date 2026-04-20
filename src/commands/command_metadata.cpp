#include "primebds/commands/command_metadata.h"

#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace primebds {
namespace {

/// All vanilla enchantment IDs, alphabetically sorted. Sourced from Endstone's
/// `enchantment.h` constants. Update this list when new enchantments are added.
constexpr const char *ENCHANT_USAGE =
    "/enchantforce <player: player> "
    "(aqua_affinity|bane_of_arthropods|binding|blast_protection|breach|channeling|density|"
    "depth_strider|efficiency|feather_falling|fire_aspect|fire_protection|flame|fortune|"
    "frost_walker|impaling|infinity|knockback|looting|loyalty|luck_of_the_sea|lunge|lure|"
    "mending|multishot|piercing|power|projectile_protection|protection|punch|quick_charge|"
    "respiration|riptide|sharpness|silk_touch|smite|soul_speed|swift_sneak|thorns|"
    "unbreaking|vanishing|wind_burst)<enchantment: enchantment> [level: int]";

/// Endstone re-registers an enum every time the same `<name: type>` appears in
/// any usage string, auto-suffixing duplicates (`pbds_action_1`, `rank_sub_2`...)
/// and emitting a warning per occurrence. To suppress those warnings, we
/// pre-emptively rename every duplicate occurrence within a single command's
/// usage list to a unique name (matching what Endstone would produce silently).
/// Both forms are rewritten: `(values)<name: type>` and `(values)[name: type]`.
std::vector<std::string> uniquifyEnumNames(std::vector<std::string> usages) {
    std::map<std::string, int> counts;
    
    for (auto &u : usages) {
        std::string out;
        out.reserve(u.size());
        size_t pos = 0;
        
        while (pos < u.size()) {
            // Look for pattern: (values)<param: type> or (values)[param: type]
            size_t paren_start = u.find('(', pos);
            if (paren_start == std::string::npos) {
                out.append(u, pos, std::string::npos);
                break;
            }
            
            size_t paren_end = u.find(')', paren_start);
            if (paren_end == std::string::npos) {
                out.append(u, pos, std::string::npos);
                break;
            }
            
            // Check if next char is < or [
            size_t bracket_pos = paren_end + 1;
            if (bracket_pos >= u.size()) {
                out.append(u, pos, paren_end + 1 - pos);
                pos = paren_end + 1;
                continue;
            }
            
            char lbracket = u[bracket_pos];
            if (lbracket != '<' && lbracket != '[') {
                out.append(u, pos, paren_end + 1 - pos);
                pos = paren_end + 1;
                continue;
            }
            
            char rbracket = (lbracket == '<') ? '>' : ']';
            size_t bracket_end = u.find(rbracket, bracket_pos);
            if (bracket_end == std::string::npos) {
                out.append(u, pos, std::string::npos);
                break;
            }
            
            // Extract content between brackets: "param: type"
            std::string content = u.substr(bracket_pos + 1, bracket_end - bracket_pos - 1);
            size_t colon = content.find(':');
            if (colon == std::string::npos) {
                out.append(u, pos, bracket_end + 1 - pos);
                pos = bracket_end + 1;
                continue;
            }
            
            // Parse param and type
            size_t param_start = content.find_first_not_of(" \t", 0);
            size_t param_end = content.find_last_not_of(" \t", colon - 1);
            size_t type_start = content.find_first_not_of(" \t", colon + 1);
            size_t type_end = content.find_last_not_of(" \t");
            
            if (param_start == std::string::npos || type_start == std::string::npos) {
                out.append(u, pos, bracket_end + 1 - pos);
                pos = bracket_end + 1;
                continue;
            }
            
            std::string param = content.substr(param_start, param_end - param_start + 1);
            std::string ename = content.substr(type_start, type_end - type_start + 1);
            std::string values = u.substr(paren_start + 1, paren_end - paren_start - 1);
            
            // Uniquify the enum name
            int n = counts[ename]++;
            std::string final_name = (n == 0) ? ename : (ename + "_" + std::to_string(n));
            
            // Append everything
            out.append(u, pos, paren_start - pos);
            out += "(" + values + ")";
            out += lbracket;
            out += param + ": " + final_name;
            out += rbracket;
            
            pos = bracket_end + 1;
        }
        
        u = std::move(out);
    }
    
    return usages;
}

struct CmdBuilder {
    endstone::detail::CommandBuilder &inner;

    CmdBuilder(endstone::detail::PluginDescriptionBuilder &b, const std::string &name)
        : inner(b.command(name)) {}

    CmdBuilder &description(const std::string &d) {
        inner.description(d);
        return *this;
    }

    template <typename... Usage>
    CmdBuilder &usages(Usage... us) {
        std::vector<std::string> v{std::string(us)...};
        v = uniquifyEnumNames(std::move(v));
        for (auto &s : v) inner.usages(s);
        return *this;
    }

    template <typename... A>
    CmdBuilder &permissions(A... a) {
        inner.permissions(a...);
        return *this;
    }

    template <typename... A>
    CmdBuilder &aliases(A... a) {
        inner.aliases(a...);
        return *this;
    }
};

CmdBuilder cmd(endstone::detail::PluginDescriptionBuilder &b, const std::string &name) {
    return CmdBuilder(b, name);
}

} // namespace

void registerEndstoneCommands(endstone::detail::PluginDescriptionBuilder &b) {

    // -----------------------------------------------------------------------
    // GAMEMODE
    // -----------------------------------------------------------------------
    cmd(b, "gma").description("Sets your game mode to adventure!")
        .usages("/gma [player: player]")
        .permissions("primebds.command.gma");

    cmd(b, "gmc").description("Sets your game mode to creative!")
        .usages("/gmc [player: player]")
        .permissions("primebds.command.gmc");

    cmd(b, "gms").description("Sets your game mode to survival!")
        .usages("/gms [player: player]")
        .permissions("primebds.command.gms");

    cmd(b, "gmsp").description("Sets your game mode to spectator!")
        .usages("/gmsp [player: player]")
        .permissions("primebds.command.gmsp");

    cmd(b, "gmt").description("Toggles you between survival and creative mode!")
        .usages("/gmt [player: player]")
        .permissions("primebds.command.gmt");

    // -----------------------------------------------------------------------
    // ITEM
    // -----------------------------------------------------------------------
    cmd(b, "enchantforce").description("Forces a given enchantment onto an item!")
        .usages(ENCHANT_USAGE)
        .permissions("primebds.command.enchantforce")
        .aliases("enchantf", "forceenchant");

    cmd(b, "giveforce").description("Forces any item registered to be given, hidden or not!")
        .usages(
            "/giveforce <player: player> <blockItem: block> [amount: int] [data: int]",
            "/giveforce <player: player> <item: string> [amount: int] [data: int]",
            "/giveforce <player: player> (glowingobsidian|netherreactor|portal|end_portal|end_gateway|frosted_ice|enchanted_book|spawn_egg|info_update|info_update2|reserved6|unknown|client_request_placeholder_block|moving_block|stonecutter|camera|glow_stick)<hiddenItem: hiddenItem> [amount: int] [data: int]")
        .permissions("primebds.command.giveforce")
        .aliases("givef", "forcegive");

    cmd(b, "hat").description("Sets the item in-hand as a hat!")
        .usages("/hat [player: player]")
        .permissions("primebds.command.hat", "primebds.command.hat.other");

    cmd(b, "iteminfo").description("Check item data!")
        .usages("/iteminfo")
        .permissions("primebds.command.iteminfo");

    cmd(b, "itemlore").description("Modify item lore data!")
        .usages("/itemlore <player: player> (add|set|delete|clear)<action: lore_action> [text: string]")
        .permissions("primebds.command.itemlore")
        .aliases("lore");

    cmd(b, "itemname").description("Modify item name data!")
        .usages("/itemname <player: player> (set|clear)<action: name_action> [name: string]")
        .permissions("primebds.command.itemname");

    cmd(b, "itemtag").description("Modify item tags!")
        .usages("/itemtag <player: player> (unbreakable)<tag: item_tag> <value: bool>")
        .permissions("primebds.command.itemtag");

    cmd(b, "more").description("Sets a full stack to the held item!")
        .usages("/more")
        .permissions("primebds.command.more");

    cmd(b, "repair").description("Repairs the item in hand!")
        .usages("/repair [player: player]")
        .permissions("primebds.command.repair", "primebds.command.repair.other");

    // -----------------------------------------------------------------------
    // JAVA PARODY
    // -----------------------------------------------------------------------
    cmd(b, "alist").description("Manages server allowlist profiles and server allowlist!")
        .usages(
            "/alist (list|check|profiles)<action: alist_action> [args: message]",
            "/alist (add|remove)<action: alist_action> <player: string> [ignore_max_player_limit: bool]",
            "/alist (create|use|delete|clear)<action: alist_action> <name: string>",
            "/alist (inherit)<action: alist_action> <child: string> <parent: string>")
        .permissions("primebds.command.alist")
        .aliases("wlist");

    cmd(b, "bossbar").description("Sets or clears a client-sided bossbar display!")
        .usages(
            "/bossbar <player: player> (red|blue|green|yellow|pink|purple|rebecca_purple|white)<color: bar_color> <percent: float> <title: message>",
            "/bossbar <player: player> (clear)<action: bar_action>")
        .permissions("primebds.command.bossbar");

    cmd(b, "spectate").description("Warp to a player to spectate them!")
        .usages("/spectate [player: player]")
        .permissions("primebds.command.spectate");

    // -----------------------------------------------------------------------
    // MESSAGE
    // -----------------------------------------------------------------------
    cmd(b, "broadcast").description("Broadcasts a message to the entire server!")
        .usages("/broadcast <message: message>")
        .permissions("primebds.command.broadcast")
        .aliases("bc");

    cmd(b, "clearchat").description("Clears the chat for all players!")
        .usages("/clearchat")
        .permissions("primebds.command.clearchat")
        .aliases("cc");

    cmd(b, "discord").description("Shows the Discord invite link!")
        .usages("/discord")
        .permissions("primebds.command.discord");

    cmd(b, "motd").description("Displays or sets the Message of the Day!")
        .usages("/motd [message: message]")
        .permissions("primebds.command.motd");

    cmd(b, "msgtoggle").description("Toggles private messages on or off!")
        .usages("/msgtoggle")
        .permissions("primebds.command.msgtoggle");

    cmd(b, "note").description("Add, remove, clear, or list notes on a player!")
        .usages(
            "/note <player: player> (add)<action: note_action> <text: message>",
            "/note <player: player> (remove)<action: note_action> <id: int>",
            "/note <player: player> (clear)<action: note_action> [args: message]",
            "/note <player: player> (list)<action: note_action> [page: int]")
        .permissions("primebds.command.note");

    cmd(b, "popup").description("Sends a popup message to a player!")
        .usages("/popup <player: player> <message: message>")
        .permissions("primebds.command.popup");

    cmd(b, "reply").description("Reply to the last person who messaged you!")
        .usages("/reply <message: message>")
        .permissions("primebds.command.reply")
        .aliases("r");

    cmd(b, "rules").description("Displays the server rules!")
        .usages("/rules")
        .permissions("primebds.command.rules");

    cmd(b, "setrules").description("Manages the server rules list!")
        .usages(
            "/setrules (add)<action: rules_action> <text: message>",
            "/setrules (edit)<action: rules_action> <index: int> <text: message>",
            "/setrules (delete)<action: rules_action> <index: int>",
            "/setrules (insert)<action: rules_action> <index: int> <text: message>",
            "/setrules (list)<action: rules_action> [page: int]")
        .permissions("primebds.command.setrules");

    cmd(b, "socialspy").description("Toggle social spy to see private messages!")
        .usages("/socialspy")
        .permissions("primebds.command.socialspy");

    cmd(b, "staffchat").description("Toggle staff chat or send a staff-only message!")
        .usages("/staffchat [message: message]")
        .permissions("primebds.command.staffchat")
        .aliases("sc");

    cmd(b, "tip").description("Sends a tip message to a player!")
        .usages("/tip <player: player> <message: message>")
        .permissions("primebds.command.tip");

    cmd(b, "toast").description("Sends a toast notification to a player!")
        .usages("/toast <player: player> <title: string> <message: message>")
        .permissions("primebds.command.toast");

    cmd(b, "voice").description("Enables voice chat attachment!")
        .usages("/voice")
        .permissions("primebds.command.voice");

    // -----------------------------------------------------------------------
    // MISC
    // -----------------------------------------------------------------------
    cmd(b, "activity").description("Lists out session information!")
        .usages("/activity <player: player> [page: int]")
        .permissions("primebds.command.activity");

    cmd(b, "activitylist").description("Lists players by activity filter!")
        .usages("/activitylist (highest|lowest|recent)<filter: activity_filter> [page: int]")
        .permissions("primebds.command.activitylist");

    cmd(b, "afk").description("Toggles AFK mode for yourself!")
        .usages("/afk")
        .permissions("primebds.command.afk");

    cmd(b, "blockinfo").description("Prints info of the facing block!")
        .usages("/blockinfo")
        .permissions("primebds.command.blockinfo");

    cmd(b, "blockscan").description("Continuously show information about the block you're looking at.")
        .usages("/blockscan [enable: bool]")
        .permissions("primebds.command.blockscan");

    cmd(b, "check").description("Checks a player's client info!")
        .usages(
            "/check <player: player>",
            "/check <player: player> (info|mod|network|world)<info: check_info>")
        .permissions("primebds.command.check")
        .aliases("seen");

    cmd(b, "cords").description("Print your current position!")
        .usages("/cords")
        .permissions("primebds.command.cords")
        .aliases("blockpos", "pos");

    cmd(b, "entityinfo").description("Check entity information!")
        .usages("/entityinfo (list)<action: entity_action> [page: int]")
        .permissions("primebds.command.entityinfo");

    cmd(b, "feed").description("Sets player hunger to full!")
        .usages("/feed [player: player]")
        .permissions("primebds.command.feed", "primebds.command.feed.other")
        .aliases("eat");

    cmd(b, "god").description("Toggles invulnerability!")
        .usages("/god [player: player] [toggle: bool]")
        .permissions("primebds.command.god", "primebds.command.god.other");

    cmd(b, "heal").description("Heals all health to full!")
        .usages("/heal [player: player]")
        .permissions("primebds.command.heal", "primebds.command.heal.other");

    cmd(b, "nickname").description("Set your display name!")
        .usages("/nickname [name: string]")
        .permissions("primebds.command.nickname")
        .aliases("nick");

    cmd(b, "ping").description("Check your or another player's latency!")
        .usages("/ping [player: player]")
        .permissions("primebds.command.ping");

    cmd(b, "playtime").description("Check total playtime!")
        .usages("/playtime [player: player]")
        .permissions("primebds.command.playtime");

    // -----------------------------------------------------------------------
    // MODERATION
    // -----------------------------------------------------------------------
    cmd(b, "alts").description("Check for alternate accounts!")
        .usages("/alts <player: player>")
        .permissions("primebds.command.alts");

    cmd(b, "altspy").description("Toggle alt detection notifications!")
        .usages("/altspy")
        .permissions("primebds.command.altspy");

    cmd(b, "globalmute").description("Toggles global mute for the server!")
        .usages("/globalmute")
        .permissions("primebds.command.globalmute")
        .aliases("gmute");

    cmd(b, "ipban").description("IP bans a player from the server!")
        .usages("/ipban <player: player> <duration: int> <unit: string> [reason: message]")
        .permissions("primebds.command.ipban");

    cmd(b, "ipmute").description("IP mutes a player on the server!")
        .usages("/ipmute <player: player> <duration: int> <unit: string> [reason: message]")
        .permissions("primebds.command.ipmute");

    cmd(b, "modspy").description("Toggle moderation spy notifications!")
        .usages("/modspy")
        .permissions("primebds.command.modspy");

    cmd(b, "mute").description("Permanently mutes a player on the server!")
        .usages("/mute <player: player> [reason: message]")
        .permissions("primebds.command.mute");

    cmd(b, "nameban").description("Bans a player name from the server!")
        .usages("/nameban <name: string> <duration: int> <unit: string> [reason: message]")
        .permissions("primebds.command.nameban");

    cmd(b, "nameunban").description("Unbans a player name from the server!")
        .usages("/nameunban <name: string>")
        .permissions("primebds.command.nameunban");

    cmd(b, "permban").description("Permanently bans a player from the server!")
        .usages("/permban <player: player> [reason: message]")
        .permissions("primebds.command.permban");

    cmd(b, "punishments").description("View a player's punishment history!")
        .usages("/punishments <player: player> [page: int]")
        .permissions("primebds.command.punishments");

    cmd(b, "removeban").description("Removes a ban from a player!")
        .usages("/removeban <player: player>")
        .permissions("primebds.command.removeban")
        .aliases("unban");

    cmd(b, "silentmute").description("Silently mutes a player (messages appear only to them)!")
        .usages("/silentmute <player: player>")
        .permissions("primebds.command.silentmute")
        .aliases("smute");

    cmd(b, "tempban").description("Temporarily bans a player from the server!")
        .usages("/tempban <player: player> <duration: int> <unit: string> [reason: message]")
        .permissions("primebds.command.tempban");

    cmd(b, "tempmute").description("Temporarily mutes a player on the server!")
        .usages("/tempmute <player: player> <duration: int> <unit: string> [reason: message]")
        .permissions("primebds.command.tempmute");

    cmd(b, "unmute").description("Removes an active mute from a player!")
        .usages("/unmute <player: player>")
        .permissions("primebds.command.unmute");

    cmd(b, "unwarn").description("Remove a warning or clear all warnings from a player!")
        .usages(
            "/unwarn <player: player> (clear)<action: unwarn_action> [args: message]",
            "/unwarn <player: player> [id: int]")
        .permissions("primebds.command.unwarn");

    cmd(b, "warn").description("Warn a player that they are breaking a rule!")
        .usages("/warn <player: player> <reason: string> [duration: int] [unit: string]")
        .permissions("primebds.command.warn");

    cmd(b, "warnings").description("List or delete warnings for a player!")
        .usages(
            "/warnings <player: player> [page: int]",
            "/warnings <player: player> (delete|clear)<action: warn_action> <id: int>")
        .permissions("primebds.command.warnings");

    // -----------------------------------------------------------------------
    // MOVEMENT
    // -----------------------------------------------------------------------
    cmd(b, "back").description("Teleport to your last saved location!")
        .usages("/back")
        .permissions("primebds.command.back");

    cmd(b, "bottom").description("Teleport to the lowest safe position!")
        .usages("/bottom")
        .permissions("primebds.command.bottom");

    cmd(b, "fly").description("Toggles flight for a player!")
        .usages("/fly [player: player]")
        .permissions("primebds.command.fly");

    cmd(b, "home").description("Manage and warp to homes!")
        .usages(
            "/home",
            "/home (set)<action: home_action> [name: string]",
            "/home (list)<action: home_action> [page: int]",
            "/home (warp)<action: home_action> <name: string>",
            "/home (del|delete)<action: home_action> <name: string>",
            "/home (max)<action: home_action> [limit: int]")
        .permissions("primebds.command.home");

    cmd(b, "homeother").description("Manage another player's homes!")
        .usages(
            "/homeother <player: player>",
            "/homeother <player: player> (list)<action: homeother_action> [page: int]",
            "/homeother <player: player> (warp)<action: homeother_action> <name: string>",
            "/homeother <player: player> (set)<action: homeother_action> [name: string]",
            "/homeother <player: player> (del)<action: homeother_action> <name: string>",
            "/homeother <player: player> (max)<action: homeother_action> [limit: int]")
        .permissions("primebds.command.homeother");

    cmd(b, "offlinetp").description("Teleport to where a player last logged out.")
        .usages("/offlinetp <player: player>")
        .permissions("primebds.command.offlinetp")
        .aliases("otp");

    cmd(b, "setback").description("Sets the global cooldown and delay for /back!")
        .usages("/setback [delay: int] [cooldown: int]")
        .permissions("primebds.command.setback");

    cmd(b, "sethomes").description("Set global home settings (delay, cooldown, cost)!")
        .usages("/sethomes <delay: int> <cooldown: int> <cost: int>")
        .permissions("primebds.command.sethomes");

    cmd(b, "setspawn").description("Sets the global spawn point!")
        .usages("/setspawn")
        .permissions("primebds.command.setspawn");

    cmd(b, "spawn").description("Warps you to the spawn!")
        .usages("/spawn")
        .permissions("primebds.command.spawn");

    cmd(b, "speed").description("Modifies player flyspeed or walkspeed!")
        .usages(
            "/speed <value: float>",
            "/speed (flyspeed|walkspeed)<mode: speed_mode> <value: float> [player: player]",
            "/speed (reset)<action: speed_action> <type: string> [player: player]")
        .permissions("primebds.command.speed");

    cmd(b, "top").description("Warps you to the topmost block with air!")
        .usages("/top")
        .permissions("primebds.command.top");

    cmd(b, "warp").description("Warp to a warp location!")
        .usages(
            "/warp",
            "/warp <name: message>",
            "/warp (list)<action: warp_action> [page: int]")
        .permissions("primebds.command.warp");

    cmd(b, "warps").description("Manage server warps!")
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
    cmd(b, "filterlist").description("Lists all players with a filter!")
        .usages("/filterlist (ops|default|online|offline|muted|banned|ipbanned)<plist_filter: plist_filter> [page: int]")
        .permissions("primebds.command.filterlist")
        .aliases("flist");

    cmd(b, "monitor").description("Monitor server performance in real time!")
        .usages("/monitor [enable: bool]")
        .permissions("primebds.command.monitor");

    cmd(b, "permissions").description("Sets the internal permissions for a player!")
        .usages("/permissions <player: player> (settrue|setfalse|setneutral)<set_perm: set_perm> <permission: message>")
        .permissions("primebds.command.permissions")
        .aliases("perms");

    cmd(b, "permissionslist").description("View the global or player-specific permissions list!")
        .usages(
            "/permissionslist",
            "/permissionslist <player: player>")
        .permissions("primebds.command.permissionslist")
        .aliases("permslist");

    cmd(b, "primebds").description("PrimeBDS management command!")
        .usages("/primebds (config|command|info|reloadconfig)[action: pbds_action]")
        .permissions("primebds.command.primebds");

    cmd(b, "rank").description("Manage server ranks!")
        .usages(
            "/rank (set)<sub: rank_sub> <player: string> <rank: string>",
            "/rank (create)<sub: rank_sub> <name: string>",
            "/rank (delete)<sub: rank_sub> <name: string>",
            "/rank (info)<sub: rank_sub> <name: string>",
            "/rank (perm)<sub: rank_sub> <action: string> <rank: string> <permission: string> [state: bool]",
            "/rank (list)<sub: rank_sub> [page: int]",
            "/rank (inherit)<sub: rank_sub> <action: string> <rank: string> <parent: string>",
            "/rank (weight)<sub: rank_sub> <rank: string> <weight: int>",
            "/rank (prefix)<sub: rank_sub> <rank: string> <prefix: message>",
            "/rank (suffix)<sub: rank_sub> <rank: string> <suffix: message>")
        .permissions("primebds.command.rank");

    cmd(b, "reloadscripts").description("Reloads server scripts!")
        .usages("/reloadscripts")
        .permissions("primebds.command.reloadscripts")
        .aliases("rscripts", "rs");

    cmd(b, "send").description("Send players to another server!")
        .usages("/send <player: player> <address: message>")
        .permissions("primebds.command.send");

    cmd(b, "updatepacks").description("Update resource or behavior pack versions!")
        .usages("/updatepacks (resource|behavior)<type: pack_type> [version: string]")
        .permissions("primebds.command.updatepacks");
}

} // namespace primebds
