/// @file rank.cpp
/// Manage server ranks!

#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"
#include "primebds/utils/config/config_manager.h"
#include "primebds/utils/permissions/permission_manager.h"

#include <algorithm>

namespace primebds::commands {

    static bool cmd_rank(PrimeBDS &, endstone::CommandSender &,
                        const std::vector<std::string> &);

    REGISTER_COMMAND(rank, "Manage server ranks!", cmd_rank,
                     info.usages = {
                         "/rank (set)<sub: rank_sub> <player: string> <rank: string>",
                         "/rank (create)<sub: rank_sub> <name: string>",
                         "/rank (delete)<sub: rank_sub> <name: string>",
                         "/rank (info)<sub: rank_sub> <name: string>",
                         "/rank (perm)<sub: rank_sub> <action: string> <rank: string> <permission: string> [state: bool]",
                         "/rank (list)<sub: rank_sub> [page: int]",
                         "/rank (inherit)<sub: rank_sub> <action: string> <rank: string> <parent: string>",
                         "/rank (weight)<sub: rank_sub> <rank: string> <weight: int>",
                         "/rank (prefix)<sub: rank_sub> <rank: string> <prefix: message>",
                         "/rank (suffix)<sub: rank_sub> <rank: string> <suffix: message>"};
                     info.permissions = {"primebds.command.rank"};);

    static std::string toLower(const std::string &s) {
        std::string out = s;
        std::transform(out.begin(), out.end(), out.begin(), ::tolower);
        return out;
    }

    /// Find the actual key in the permissions JSON (case-insensitive)
    static std::string findRankKey(const nlohmann::json &perms, const std::string &name) {
        auto name_lower = toLower(name);
        for (auto &[key, val] : perms.items()) {
            if (toLower(key) == name_lower)
                return key;
        }
        return "";
    }

    /// Manage server ranks!
    static bool cmd_rank(PrimeBDS &plugin, endstone::CommandSender &sender,
                         const std::vector<std::string> &args) {
        if (args.empty()) {
            sender.sendMessage("\u00a7cUsage: /rank <set|create|delete|info|perm|list|inherit|weight|prefix|suffix> ...");
            return false;
        }

        auto &cfg = config::ConfigManager::instance();
        auto &pm = permissions::PermissionManager::instance();
        std::string sub = toLower(args[0]);

        if (sub == "list") {
            auto perms = cfg.loadPermissions();
            if (perms.empty()) {
                sender.sendMessage("\u00a7cNo ranks exist");
                return true;
            }
            sender.sendMessage("\u00a7a--- Ranks ---");
            for (auto &[name, data] : perms.items()) {
                int weight = data.value("weight", 0);
                std::string prefix = data.value("prefix", "");
                sender.sendMessage("\u00a7e" + name + " \u00a77[weight: " + std::to_string(weight) + "]" +
                                   (prefix.empty() ? "" : " \u00a77prefix: " + prefix));
            }
            return true;
        }

        if (sub == "create" && args.size() >= 2) {
            std::string name = args[1];
            auto perms = cfg.loadPermissions();
            if (!findRankKey(perms, name).empty()) {
                sender.sendMessage("\u00a7cRank \u00a7e" + name + " \u00a7calready exists");
                return true;
            }
            perms[name] = {{"permissions", nlohmann::json::object()}, {"inherits", nlohmann::json::array()}, {"weight", 0}};
            cfg.savePermissions(perms);
            pm.loadPermissions(plugin.getServer());
            sender.sendMessage("\u00a7aRank \u00a7e" + name + " \u00a7acreated");
            return true;
        }

        if (sub == "delete" && args.size() >= 2) {
            auto perms = cfg.loadPermissions();
            auto key = findRankKey(perms, args[1]);
            if (key.empty()) {
                sender.sendMessage("\u00a7cRank \u00a7e" + args[1] + " \u00a7cdoes not exist");
                return true;
            }
            if (toLower(key) == "default") {
                sender.sendMessage("\u00a7cCannot delete the Default rank");
                return true;
            }
            perms.erase(key);
            cfg.savePermissions(perms);
            pm.loadPermissions(plugin.getServer());
            sender.sendMessage("\u00a7aRank \u00a7e" + key + " \u00a7adeleted");
            return true;
        }

        if (sub == "set" && args.size() >= 3) {
            std::string player_name = args[1];
            std::string rank_name = args[2];
            auto *target = plugin.getServer().getPlayer(player_name);
            if (!target) {
                sender.sendMessage("\u00a7cPlayer \u00a7e" + player_name + " \u00a7cnot found online");
                return false;
            }
            auto perms = cfg.loadPermissions();
            auto key = findRankKey(perms, rank_name);
            if (key.empty()) {
                sender.sendMessage("\u00a7cRank \u00a7e" + rank_name + " \u00a7cdoes not exist");
                return false;
            }
            plugin.db->setUserRank(target->getXuid(), key);
            plugin.reloadCustomPerms(*target);
            sender.sendMessage("\u00a7e" + player_name + " \u00a7arank set to \u00a7e" + key);
            return true;
        }

        if (sub == "info" && args.size() >= 2) {
            auto perms = cfg.loadPermissions();
            auto key = findRankKey(perms, args[1]);
            if (key.empty()) {
                sender.sendMessage("\u00a7cRank \u00a7e" + args[1] + " \u00a7cdoes not exist");
                return true;
            }
            auto &data = perms[key];
            sender.sendMessage("\u00a7a--- Rank: " + key + " ---");
            sender.sendMessage("\u00a77Weight: \u00a7e" + std::to_string(data.value("weight", 0)));
            std::string prefix = data.value("prefix", "");
            std::string suffix = data.value("suffix", "");
            if (!prefix.empty())
                sender.sendMessage("\u00a77Prefix: \u00a7r" + prefix);
            if (!suffix.empty())
                sender.sendMessage("\u00a77Suffix: \u00a7r" + suffix);
            if (data.contains("inherits") && data["inherits"].is_array() && !data["inherits"].empty()) {
                std::string inh;
                for (auto &p : data["inherits"]) {
                    if (!inh.empty()) inh += ", ";
                    inh += p.get<std::string>();
                }
                sender.sendMessage("\u00a77Inherits: \u00a7e" + inh);
            }
            if (data.contains("permissions") && data["permissions"].is_object() && !data["permissions"].empty()) {
                sender.sendMessage("\u00a77Permissions:");
                for (auto &[pname, pval] : data["permissions"].items()) {
                    std::string status = pval.get<bool>() ? "\u00a7atrue" : "\u00a7cfalse";
                    sender.sendMessage("  \u00a7b" + pname + " \u00a77= " + status);
                }
            }
            return true;
        }

        if (sub == "perm" && args.size() >= 4) {
            std::string action = toLower(args[1]);
            std::string rank_name = args[2];
            std::string perm = args[3];
            auto perms = cfg.loadPermissions();
            auto key = findRankKey(perms, rank_name);
            if (key.empty()) {
                sender.sendMessage("\u00a7cRank \u00a7e" + rank_name + " \u00a7cdoes not exist");
                return false;
            }
            if (!perms[key].contains("permissions") || !perms[key]["permissions"].is_object())
                perms[key]["permissions"] = nlohmann::json::object();

            if (action == "add") {
                perms[key]["permissions"][perm] = true;
                cfg.savePermissions(perms);
                pm.clearPrefixSuffixCache();
                for (auto *p : plugin.getServer().getOnlinePlayers()) {
                    auto u = plugin.db->getOnlineUser(p->getXuid());
                    if (u && toLower(u->internal_rank) == toLower(key))
                        plugin.reloadCustomPerms(*p);
                }
                sender.sendMessage("\u00a7aPermission \u00a7e" + perm + " \u00a7aadded to rank \u00a7e" + key);
            } else if (action == "remove") {
                perms[key]["permissions"].erase(perm);
                cfg.savePermissions(perms);
                pm.clearPrefixSuffixCache();
                for (auto *p : plugin.getServer().getOnlinePlayers()) {
                    auto u = plugin.db->getOnlineUser(p->getXuid());
                    if (u && toLower(u->internal_rank) == toLower(key))
                        plugin.reloadCustomPerms(*p);
                }
                sender.sendMessage("\u00a7aPermission \u00a7e" + perm + " \u00a7aremoved from rank \u00a7e" + key);
            }
            return true;
        }

        if (sub == "inherit" && args.size() >= 3) {
            auto perms = cfg.loadPermissions();
            auto key = findRankKey(perms, args[1]);
            auto parent_key = findRankKey(perms, args[2]);
            if (key.empty()) {
                sender.sendMessage("\u00a7cRank \u00a7e" + args[1] + " \u00a7cdoes not exist");
                return false;
            }
            if (parent_key.empty()) {
                sender.sendMessage("\u00a7cParent rank \u00a7e" + args[2] + " \u00a7cdoes not exist");
                return false;
            }
            if (!perms[key].contains("inherits") || !perms[key]["inherits"].is_array())
                perms[key]["inherits"] = nlohmann::json::array();
            // Add if not already present
            bool found = false;
            for (auto &p : perms[key]["inherits"]) {
                if (toLower(p.get<std::string>()) == toLower(parent_key)) {
                    found = true;
                    break;
                }
            }
            if (!found)
                perms[key]["inherits"].push_back(parent_key);
            cfg.savePermissions(perms);
            sender.sendMessage("\u00a7aRank \u00a7e" + key + " \u00a7anow inherits from \u00a7e" + parent_key);
            return true;
        }

        if (sub == "weight" && args.size() >= 3) {
            auto perms = cfg.loadPermissions();
            auto key = findRankKey(perms, args[1]);
            if (key.empty()) {
                sender.sendMessage("\u00a7cRank \u00a7e" + args[1] + " \u00a7cdoes not exist");
                return false;
            }
            int weight = std::atoi(args[2].c_str());
            perms[key]["weight"] = weight;
            cfg.savePermissions(perms);
            sender.sendMessage("\u00a7aRank \u00a7e" + key + " \u00a7aweight set to \u00a7e" + std::to_string(weight));
            return true;
        }

        if (sub == "prefix" && args.size() >= 3) {
            auto perms = cfg.loadPermissions();
            auto key = findRankKey(perms, args[1]);
            if (key.empty()) {
                sender.sendMessage("\u00a7cRank \u00a7e" + args[1] + " \u00a7cdoes not exist");
                return false;
            }
            std::string prefix;
            for (size_t i = 2; i < args.size(); ++i) {
                if (i > 2) prefix += " ";
                prefix += args[i];
            }
            perms[key]["prefix"] = prefix;
            cfg.savePermissions(perms);
            pm.clearPrefixSuffixCache();
            sender.sendMessage("\u00a7aRank \u00a7e" + key + " \u00a7aprefix set to \u00a7r" + prefix);
            return true;
        }

        if (sub == "suffix" && args.size() >= 3) {
            auto perms = cfg.loadPermissions();
            auto key = findRankKey(perms, args[1]);
            if (key.empty()) {
                sender.sendMessage("\u00a7cRank \u00a7e" + args[1] + " \u00a7cdoes not exist");
                return false;
            }
            std::string suffix;
            for (size_t i = 2; i < args.size(); ++i) {
                if (i > 2) suffix += " ";
                suffix += args[i];
            }
            perms[key]["suffix"] = suffix;
            cfg.savePermissions(perms);
            pm.clearPrefixSuffixCache();
            sender.sendMessage("\u00a7aRank \u00a7e" + key + " \u00a7asuffix set to \u00a7r" + suffix);
            return true;
        }

        sender.sendMessage("\u00a7cInvalid subcommand for /rank");
        return false;
    }

} // namespace primebds::commands
/// @file rank.cpp
