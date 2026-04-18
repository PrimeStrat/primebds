/// @file command_intercept.cpp
/// Command preprocessing: moderation command remapping, exemption checks.

#include "primebds/handlers/preprocesses/command_intercept.h"
#include "primebds/handlers/preprocesses/crasher_patch.h"
#include "primebds/handlers/preprocesses/whisper.h"
#include "primebds/plugin.h"
#include "primebds/utils/config/config_manager.h"
#include "primebds/utils/logging.h"
#include "primebds/utils/permissions/permission_manager.h"
#include "primebds/utils/target_selector.h"

#include <algorithm>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace primebds::handlers::preprocesses {

    static const std::set<std::string> MODERATION_COMMANDS = {
        "kick", "ban", "pardon", "unban", "permban", "tempban", "tempmute",
        "mute", "ipban", "unmute", "warn", "banlist", "ban-ip", "unban-ip",
        "pardon-ip"};

    static const std::set<std::string> MSG_CMDS = {
        "me", "tell", "w", "whisper", "msg", "say"};

    static std::set<std::string> PARSE_COMMANDS;

    static void initParseCommands() {
        if (!PARSE_COMMANDS.empty())
            return;
        PARSE_COMMANDS.insert(MODERATION_COMMANDS.begin(), MODERATION_COMMANDS.end());
        PARSE_COMMANDS.insert(MSG_CMDS.begin(), MSG_CMDS.end());
        PARSE_COMMANDS.insert({"op", "deop", "allowlist", "whitelist", "transfer",
                               "teleport", "tp", "stop"});
    }

    static std::vector<std::string> splitCommand(const std::string &command) {
        std::vector<std::string> args;
        std::istringstream iss(command);
        std::string token;
        bool in_quotes = false;
        std::string current;

        for (size_t i = 0; i < command.size(); ++i) {
            char c = command[i];
            if (c == '"') {
                in_quotes = !in_quotes;
            } else if (c == ' ' && !in_quotes) {
                if (!current.empty()) {
                    args.push_back(current);
                    current.clear();
                }
            } else {
                current += c;
            }
        }
        if (!current.empty())
            args.push_back(current);
        return args;
    }

    static std::string toLower(const std::string &s) {
        std::string result = s;
        std::transform(result.begin(), result.end(), result.begin(), ::tolower);
        return result;
    }

    void handleCommandPreprocess(PrimeBDS &plugin, endstone::PlayerCommandEvent &event) {
        initParseCommands();

        auto &player = event.getPlayer();
        std::string command = event.getCommand();
        auto args = splitCommand(command);
        if (args.empty())
            return;

        auto &cfg = config::ConfigManager::instance();
        auto conf = cfg.config();
        auto &modules = conf["modules"];

        // Command logs
        if (modules.value("/discord_webhook/command_logs/enabled"_json_pointer, false)) {
            utils::discordRelay("**" + player.getName() + "** ran: " + command, "cmd");
        }

        std::string cmd = toLower(args[0]);
        if (!cmd.empty() && cmd[0] == '/')
            cmd = cmd.substr(1);

        if (PARSE_COMMANDS.find(cmd) == PARSE_COMMANDS.end())
            return;

        bool mc_enabled = modules.value("/permissions_manager/minecraft"_json_pointer, true);
        bool es_enabled = modules.value("/permissions_manager/endstone"_json_pointer, true);

        // Crasher patch for MSG commands
        if (MSG_CMDS.count(cmd) && checkCrasherExploit(plugin, player, command)) {
            event.setCancelled(true);
            return;
        }

        // Exemption check for moderation commands
        if (MODERATION_COMMANDS.count(cmd) && args.size() >= 2 && args[1].find('@') == std::string::npos) {
            auto target = plugin.db->getUserByName(args[1]);
            if (target.has_value()) {
                // Map command to its exemption permission
                std::string exempt_perm;
                if (cmd == "warn")
                    exempt_perm = "primebds.exempt.warn";
                else if (cmd == "kick")
                    exempt_perm = "primebds.exempt.kick";
                else if (cmd == "mute" || cmd == "tempmute")
                    exempt_perm = "primebds.exempt.mute";
                else if (cmd == "permban" || cmd == "tempban" || cmd == "ipban" ||
                         cmd == "ban" || cmd == "ban-ip")
                    exempt_perm = "primebds.exempt.ban";

                if (!exempt_perm.empty()) {
                    bool is_exempt = false;
                    auto *tp = plugin.getServer().getPlayer(target->name);
                    if (tp) {
                        is_exempt = tp->hasPermission(exempt_perm);
                    } else {
                        // Offline: check rank permissions directly
                        auto &pm = permissions::PermissionManager::instance();
                        auto rank_perms = pm.getRankPermissions(target->internal_rank);
                        auto it = rank_perms.find(exempt_perm);
                        is_exempt = (it != rank_perms.end() && it->second);
                    }

                    if (is_exempt) {
                        player.sendMessage("\u00a76Player \u00a7e" + target->name +
                                           " \u00a76is exempt from \u00a7e" + cmd);
                        event.setCancelled(true);
                        return;
                    }
                }
            }
        }

        // TP bypass
        if ((cmd == "teleport" || cmd == "tp") && mc_enabled &&
            player.hasPermission("minecraft.command.teleport") && !player.isOp()) {
            (void)plugin.getServer().dispatchCommand(plugin.getServer().getCommandSender(),
                                                     "execute as \"" + player.getName() + "\" at \"" + player.getName() +
                                                         "\" run " + command);
            event.setCancelled(true);
            return;
        }

        // Kick with selector support
        if (cmd == "kick" && args.size() >= 2) {
            std::string selector = args[1];
            std::string reason;
            for (size_t i = 2; i < args.size(); ++i) {
                if (!reason.empty())
                    reason += " ";
                reason += args[i];
            }

            auto matched = utils::getMatchingActors(plugin.getServer(), selector, player);
            if (matched.empty())
                return;

            for (auto *actor : matched) {
                auto *target_player = dynamic_cast<endstone::Player *>(actor);
                if (!target_player)
                    continue;

                auto target = plugin.db->getOnlineUser(target_player->getXuid());
                if (target.has_value() && target_player->hasPermission("primebds.exempt.kick")) {
                    player.sendMessage("\u00a76Player \u00a7e" + target->name +
                                       " \u00a76is exempt from \u00a7ekick");
                    continue;
                }
                target_player->kick(reason);
                player.sendMessage("\u00a76Player \u00a7e" + target_player->getName() +
                                   " \u00a76was kicked for \u00a7e\"" + reason + "\"");
            }
            event.setCancelled(true);
            return;
        }

        // Stop command - kick all with shutdown message
        if (cmd == "stop") {
            std::string shutdown_msg = modules.value("/join_leave_messages/shutdown"_json_pointer,
                                                     std::string("Server was shutdown!"));
            for (auto *p : plugin.getServer().getOnlinePlayers()) {
                p->kick(shutdown_msg);
            }
            return;
        }

        // Banlist remap
        if (cmd == "banlist" && es_enabled) {
            if (args.size() < 2) {
                player.performCommand("flist banned");
                player.performCommand("flist ipbanned");
            } else {
                if (args[1] == "players")
                    player.performCommand("flist banned");
                else
                    player.performCommand("flist ipbanned");
            }
            event.setCancelled(true);
            return;
        }

        // Ban → permban remap
        if (cmd == "ban" && args.size() > 1 && es_enabled) {
            player.performCommand("permban \"" + args[1] + "\"");
            event.setCancelled(true);
            return;
        }

        // Ban-ip → ipban remap
        if (cmd == "ban-ip" && args.size() > 1 && es_enabled) {
            if (args.size() > 2)
                player.performCommand("ipban \"" + args[1] + "\" ip \"" + args[2] + "\"");
            else
                player.performCommand("ipban \"" + args[1] + "\" ip");
            event.setCancelled(true);
            return;
        }

        // Unban/pardon remap
        if ((cmd == "unban" || cmd == "pardon" || cmd == "unban-ip" || cmd == "pardon-ip") &&
            args.size() > 1 && es_enabled) {
            if (cmd == "unban-ip" || cmd == "pardon-ip")
                player.performCommand("removeban ip \"" + args[1] + "\"");
            else
                player.performCommand("removeban \"" + args[1] + "\"");
            event.setCancelled(true);
            return;
        }

        // Op → rank set operator + BDS op
        if (cmd == "op" && args.size() > 1) {
            auto *reg = CommandRegistry::instance().find("rank");
            if (reg) {
                std::vector<std::string> rank_args = {"set", args[1], "operator"};
                reg->handler(plugin, player, rank_args);
            }
            auto *target = plugin.getServer().getPlayer(args[1]);
            if (target)
                target->setOp(true);
            event.setCancelled(true);
            return;
        }

        // Deop → rank set default + BDS deop
        if (cmd == "deop" && args.size() > 1) {
            auto *reg = CommandRegistry::instance().find("rank");
            if (reg) {
                std::vector<std::string> rank_args = {"set", args[1], "default"};
                reg->handler(plugin, player, rank_args);
            }
            auto *target = plugin.getServer().getPlayer(args[1]);
            if (target)
                target->setOp(false);
            event.setCancelled(true);
            return;
        }

        // Allowlist/whitelist remap
        if ((cmd == "allowlist" || cmd == "whitelist") && args.size() > 1) {
            std::string sub = args[1];
            if ((sub == "add" || sub == "remove") && args.size() > 2)
                player.performCommand("alist " + sub + " \"" + args[2] + "\"");
            else if (sub == "list")
                player.performCommand("alist list");
            else if (sub == "on" || sub == "off")
                player.sendMessage("Mojang has this feature disabled");
            event.setCancelled(true);
            return;
        }

        // Transfer remap
        if (cmd == "transfer" && args.size() > 2) {
            std::string port = (args.size() >= 4) ? args[3] : "19132";
            player.performCommand("send \"" + args[1] + "\" " + args[2] + " " + port);
            event.setCancelled(true);
            return;
        }

        // Message commands - mute check, msg toggle, whisper, social spy
        if (MSG_CMDS.count(cmd) && args.size() > 1) {
            // Mute check
            auto mod_log = plugin.db->getModLog(player.getXuid());
            if (mod_log.has_value() && mod_log->is_muted) {
                plugin.db->checkAndUpdateMute(player.getXuid(), player.getName());
                event.setCancelled(true);
                return;
            }

            std::string target = args[1];
            if (target.find('@') != std::string::npos) {
                player.sendMessage("\u00a7cTarget selectors are invalid for this command");
                event.setCancelled(true);
                return;
            }

            if (cmd == "me")
                return;

            plugin.db->updateUser(player.getXuid(), "last_messaged", target);

            auto target_user = plugin.db->getUserByName(target);
            if (target_user.has_value()) {
                if (target_user->enabled_mt == 0 &&
                    !player.hasPermission("primebds.exempt.msgtoggle")) {
                    player.sendMessage("\u00a7cThis player has private messages disabled");
                    event.setCancelled(true);
                    return;
                }
            }

            // Build message content
            std::string message;
            for (size_t i = 2; i < args.size(); ++i) {
                if (!message.empty())
                    message += " ";
                message += args[i];
            }

            utils::discordRelay("**" + player.getName() + " -> " + target + "**: " + message, "chat");

            // Enhanced whispers
            if (modules.value("/server_messages/enhanced_whispers"_json_pointer, false)) {
                if (handleWhisperCommand(plugin, player, target, message)) {
                    event.setCancelled(true);
                }
            }

            // Social spy relay
            std::string spy_prefix = modules.value("/server_messages/social_spy_prefix"_json_pointer,
                                                   std::string("\u00a77[SocialSpy] "));
            for (auto *p : plugin.getServer().getOnlinePlayers()) {
                auto user = plugin.db->getOnlineUser(p->getXuid());
                if (user.has_value() && user->enabled_ss == 1 &&
                    p->hasPermission("primebds.command.socialspy")) {
                    p->sendMessage(spy_prefix + "\u00a78[\u00a7r" + player.getName() +
                                   " \u00a77-> \u00a7r" + target + "\u00a78] \u00a77" + message);
                }
            }
        }
    }

    void handleServerCommandPreprocess(PrimeBDS &plugin, endstone::ServerCommandEvent &event) {
        initParseCommands();

        std::string command = event.getCommand();
        auto args = splitCommand(command);
        if (args.empty())
            return;

        std::string cmd = toLower(args[0]);
        if (!cmd.empty() && cmd[0] == '/')
            cmd = cmd.substr(1);

        if (PARSE_COMMANDS.find(cmd) == PARSE_COMMANDS.end())
            return;

        auto &server = plugin.getServer();
        auto &sender = server.getCommandSender();

        // Stop - kick all
        if (cmd == "stop") {
            for (auto *p : server.getOnlinePlayers()) {
                p->kick("Server was shutdown!");
            }
            return;
        }

        // Simple remap commands
        if (cmd == "ban" && args.size() > 1) {
            (void)server.dispatchCommand(sender, "permban " + args[1]);
            event.setCancelled(true);
            return;
        }
        if ((cmd == "unban" || cmd == "pardon") && args.size() > 1) {
            (void)server.dispatchCommand(sender, "removeban " + args[1]);
            event.setCancelled(true);
            return;
        }
        if (cmd == "op" && args.size() > 1) {
            auto *reg = CommandRegistry::instance().find("rank");
            if (reg) {
                std::vector<std::string> rank_args = {"set", args[1], "operator"};
                reg->handler(plugin, sender, rank_args);
            }
            auto *target = server.getPlayer(args[1]);
            if (target)
                target->setOp(true);
            event.setCancelled(true);
            return;
        }
        if (cmd == "deop" && args.size() > 1) {
            auto *reg = CommandRegistry::instance().find("rank");
            if (reg) {
                std::vector<std::string> rank_args = {"set", args[1], "default"};
                reg->handler(plugin, sender, rank_args);
            }
            auto *target = server.getPlayer(args[1]);
            if (target)
                target->setOp(false);
            event.setCancelled(true);
            return;
        }

        // Allowlist/whitelist
        if ((cmd == "allowlist" || cmd == "whitelist") && args.size() > 1) {
            if ((args[1] == "add" || args[1] == "remove") && args.size() > 2)
                (void)server.dispatchCommand(sender, "alist " + args[1] + " \"" + args[2] + "\"");
            else if (args[1] == "on" || args[1] == "off" || args[1] == "list")
                (void)server.dispatchCommand(sender, "alist " + args[1]);
            event.setCancelled(true);
            return;
        }

        // Transfer
        if (cmd == "transfer" && args.size() >= 3) {
            std::string port = (args.size() >= 4) ? args[3] : "19132";
            (void)server.dispatchCommand(sender, "send \"" + args[1] + "\" " + args[2] + " " + port);
            event.setCancelled(true);
        }
    }

} // namespace primebds::handlers::preprocesses
