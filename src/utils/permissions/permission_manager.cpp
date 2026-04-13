/// @file permission_manager.cpp
/// Rank-based permission system implementation.

#include "primebds/utils/permissions/permission_manager.h"
#include "primebds/utils/config/config_manager.h"

#include <algorithm>
#include <iostream>
#include <set>

namespace primebds::permissions
{

    static const std::vector<std::string> MINECRAFT_PERMISSIONS = {
        "minecraft", "minecraft.command", "minecraft.command.aimassist",
        "minecraft.command.allowlist", "minecraft.command.camera",
        "minecraft.command.camerashake", "minecraft.command.clear",
        "minecraft.command.clearspawnpoint", "minecraft.command.clone",
        "minecraft.command.damage", "minecraft.command.daylock",
        "minecraft.command.deop", "minecraft.command.dialogue",
        "minecraft.command.effect", "minecraft.command.enchant",
        "minecraft.command.event", "minecraft.command.execute",
        "minecraft.command.fill", "minecraft.command.function",
        "minecraft.command.fog", "minecraft.command.gamemode",
        "minecraft.command.gametest", "minecraft.command.gamerule",
        "minecraft.command.give", "minecraft.command.help",
        "minecraft.command.inputpermission", "minecraft.command.kill",
        "minecraft.command.locate", "minecraft.command.list",
        "minecraft.command.loot", "minecraft.command.music",
        "minecraft.command.me", "minecraft.command.mobevent",
        "minecraft.command.op", "minecraft.command.particle",
        "minecraft.command.place", "minecraft.command.playanimation",
        "minecraft.command.playsound", "minecraft.command.permissionslist",
        "minecraft.command.recipe", "minecraft.command.replaceitem",
        "minecraft.command.ride", "minecraft.command.reload",
        "minecraft.command.save", "minecraft.command.say",
        "minecraft.command.schedule", "minecraft.command.scoreboard",
        "minecraft.command.script", "minecraft.command.scriptevent",
        "minecraft.command.setblock", "minecraft.command.setworldspawn",
        "minecraft.command.spawnpoint", "minecraft.command.spreadplayers",
        "minecraft.command.stop", "minecraft.command.stopsound",
        "minecraft.command.tag", "minecraft.command.teleport",
        "minecraft.command.tell", "minecraft.command.tellraw",
        "minecraft.command.time", "minecraft.command.title",
        "minecraft.command.titleraw", "minecraft.command.tickingarea",
        "minecraft.command.transfer", "minecraft.command.toggledownfall",
        "minecraft.command.weather", "minecraft.command.wsserver",
        "minecraft.command.xp", "minecraft.command.summon",
        "minecraft.command.structure", "minecraft.command.testfor",
        "minecraft.command.testforblock", "minecraft.command.testforblocks"};

    static const std::vector<std::string> EXTRA_PERMS = {
        "primebds.exempt.msgtoggle", "primebds.exempt.globalmute",
        "primebds.exempt.mute", "primebds.exempt.ban",
        "primebds.exempt.kick", "primebds.exempt.warn",
        "primebds.exempt.homes", "primebds.exempt.homes.other",
        "primebds.exempt.home.delays", "primebds.exempt.home.cooldowns",
        "primebds.exempt.warp.delays", "primebds.exempt.warp.cooldowns",
        "primebds.exempt.spawn.delays", "primebds.exempt.spawn.cooldowns",
        "primebds.exempt.back.delays", "primebds.exempt.back.cooldowns",
        "primebds.command.heal.other", "primebds.command.feed.other",
        "primebds.command.repair.other", "primebds.command.god.other",
        "primebds.command.hat.other"};

    PermissionManager &PermissionManager::instance()
    {
        static PermissionManager inst;
        return inst;
    }

    static std::string toLower(const std::string &s)
    {
        std::string out = s;
        std::transform(out.begin(), out.end(), out.begin(), ::tolower);
        return out;
    }

    void PermissionManager::loadPermissions(endstone::Server &server)
    {
        std::lock_guard lock(mutex_);

        PERMISSIONS = config::ConfigManager::instance().loadPermissions();

        auto &cfg = config::ConfigManager::instance();
        auto modules = cfg.getModule("permissions_manager");
        bool mc_enabled = modules.value("minecraft", true);
        bool pb_enabled = modules.value("primebds", true);
        bool es_enabled = modules.value("endstone", true);
        bool wc_enabled = modules.value("*", true);

        std::set<std::string> all_perms;

        // Scan server-registered permissions
        for (auto *perm : server.getPluginManager().getPermissions())
        {
            auto name = toLower(perm->getName());
            auto prefix = name.substr(0, name.find('.'));

            if ((prefix == "minecraft" && mc_enabled) ||
                (prefix == "primebds" && pb_enabled) ||
                (prefix == "endstone" && es_enabled) ||
                (prefix != "minecraft" && prefix != "primebds" && prefix != "endstone" && wc_enabled))
            {
                all_perms.insert(name);
            }
        }

        if (mc_enabled)
        {
            for (auto &p : MINECRAFT_PERMISSIONS)
                all_perms.insert(toLower(p));
        }

        if (pb_enabled)
        {
            for (auto &p : EXTRA_PERMS)
                all_perms.insert(toLower(p));
        }

        MANAGED_PERMISSIONS_LIST.clear();
        MANAGED_PERMISSIONS_LIST.reserve(all_perms.size());
        for (auto &p : all_perms)
            MANAGED_PERMISSIONS_LIST.push_back(p);

        std::cout << "[PrimeBDS] Total managed permissions: " << MANAGED_PERMISSIONS_LIST.size() << "\n";
    }

    std::map<std::string, bool> PermissionManager::getRankPermissions(const std::string &rank)
    {
        std::lock_guard lock(mutex_);

        // Reload permissions from file
        PERMISSIONS = config::ConfigManager::instance().loadPermissions();

        // Normalize rank name
        std::string base_rank = "Default";
        auto rank_lower = toLower(rank);
        for (auto &[key, val] : PERMISSIONS.items())
        {
            if (toLower(key) == rank_lower)
            {
                base_rank = key;
                break;
            }
        }

        // Initialize all managed perms to false
        std::map<std::string, bool> result;
        for (auto &p : MANAGED_PERMISSIONS_LIST)
            result[p] = false;

        std::set<std::string> seen;

        std::function<void(const std::string &)> gather;
        gather = [&](const std::string &r)
        {
            // Find actual key
            std::string actual_key;
            auto r_lower = toLower(r);
            for (auto &[key, val] : PERMISSIONS.items())
            {
                if (toLower(key) == r_lower)
                {
                    actual_key = key;
                    break;
                }
            }
            if (actual_key.empty() || seen.count(actual_key))
                return;
            seen.insert(actual_key);

            auto &group = PERMISSIONS[actual_key];
            if (!group.is_object())
                return;

            // Process inheritance first
            if (group.contains("inherits") && group["inherits"].is_array())
            {
                for (auto &parent : group["inherits"])
                {
                    if (parent.is_string())
                        gather(parent.get<std::string>());
                }
            }

            // Process permissions
            if (group.contains("permissions"))
            {
                auto &perms = group["permissions"];
                std::map<std::string, bool> fixed;

                if (perms.is_object())
                {
                    for (auto &[k, v] : perms.items())
                        fixed[toLower(k)] = v.get<bool>();
                }
                else if (perms.is_array())
                {
                    for (auto &p : perms)
                    {
                        if (p.is_string())
                            fixed[toLower(p.get<std::string>())] = true;
                    }
                }

                // Wildcard
                auto wc = fixed.find("*");
                if (wc != fixed.end())
                {
                    bool wildcard_val = wc->second;
                    for (auto &[p, _] : result)
                    {
                        if (fixed.find(p) == fixed.end())
                            result[p] = wildcard_val;
                    }
                }

                for (auto &[perm_name, allowed] : fixed)
                {
                    if (perm_name == "*")
                        continue;
                    result[perm_name] = allowed;
                }
            }
        };

        gather(base_rank);
        return result;
    }

    bool PermissionManager::checkPermission(endstone::Player &player, const std::string &perm,
                                            const std::string &rank)
    {
        auto xuid = player.getXuid();

        // Check cache first
        {
            std::lock_guard lock(mutex_);
            auto it = perm_cache_.find(xuid);
            if (it != perm_cache_.end())
            {
                auto pit = it->second.find(toLower(perm));
                if (pit != it->second.end())
                    return pit->second;
            }
        }

        // Compute and cache
        auto rank_perms = getRankPermissions(rank);

        // Overlay user-specific permissions (from DB) would be added at the call site

        {
            std::lock_guard lock(mutex_);
            perm_cache_[xuid] = rank_perms;
        }

        auto it = rank_perms.find(toLower(perm));
        return it != rank_perms.end() && it->second;
    }

    std::string PermissionManager::getPrefix(const std::string &rank)
    {
        {
            std::lock_guard lock(mutex_);
            auto it = prefix_cache_.find(rank);
            if (it != prefix_cache_.end())
                return it->second;
        }

        std::string prefix;
        if (PERMISSIONS.contains(rank) && PERMISSIONS[rank].contains("prefix"))
            prefix = PERMISSIONS[rank]["prefix"].get<std::string>();

        std::lock_guard lock(mutex_);
        prefix_cache_[rank] = prefix;
        return prefix;
    }

    std::string PermissionManager::getSuffix(const std::string &rank)
    {
        {
            std::lock_guard lock(mutex_);
            auto it = suffix_cache_.find(rank);
            if (it != suffix_cache_.end())
                return it->second;
        }

        std::string suffix;
        if (PERMISSIONS.contains(rank) && PERMISSIONS[rank].contains("suffix"))
            suffix = PERMISSIONS[rank]["suffix"].get<std::string>();

        std::lock_guard lock(mutex_);
        suffix_cache_[rank] = suffix;
        return suffix;
    }

    std::string PermissionManager::checkRankExists(endstone::Plugin &plugin, endstone::Player &player,
                                                   const std::string &rank)
    {
        auto rank_lower = toLower(rank);
        for (auto &[key, val] : PERMISSIONS.items())
        {
            if (toLower(key) == rank_lower)
            {
                auto &group = val;
                if (group.is_object() && group.contains("permissions"))
                    return key;
            }
        }

        std::cout << "[PrimeBDS] Invalid rank '" << rank << "' for " << player.getName()
                  << ", resetting to Default.\n";
        return "Default";
    }

    void PermissionManager::clearPrefixSuffixCache()
    {
        std::lock_guard lock(mutex_);
        prefix_cache_.clear();
        suffix_cache_.clear();
    }

    void PermissionManager::invalidatePermCache(const std::string &xuid)
    {
        std::lock_guard lock(mutex_);
        perm_cache_.erase(xuid);
    }

    std::string PermissionManager::getPermissionHeader(endstone::Plugin &plugin)
    {
        const auto &desc = plugin.getDescription();

        auto perms = desc.getPermissions();
        if (perms.empty())
            return {};

        auto name = toLower(perms[0].getName());
        auto dot = name.find('.');
        return dot != std::string::npos ? name.substr(0, dot) : name;
    }

} // namespace primebds::permissions
