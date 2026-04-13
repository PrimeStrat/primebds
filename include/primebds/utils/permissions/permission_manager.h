/// @file permission_manager.h
/// Manages rank-based permissions, permission checking, and prefix/suffix display.

#pragma once

#include <endstone/endstone.hpp>
#include <nlohmann/json.hpp>
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

namespace primebds::permissions
{

    class PermissionManager
    {
    public:
        static PermissionManager &instance();

        void loadPermissions(endstone::Server &server);

        std::map<std::string, bool> getRankPermissions(const std::string &rank);
        bool checkPermission(endstone::Player &player, const std::string &perm,
                             const std::string &rank);

        std::string getPrefix(const std::string &rank);
        std::string getSuffix(const std::string &rank);
        std::string checkRankExists(endstone::Plugin &plugin, endstone::Player &player,
                                    const std::string &rank);

        void clearPrefixSuffixCache();
        void invalidatePermCache(const std::string &xuid);

        std::string getPermissionHeader(endstone::Plugin &plugin);

        nlohmann::json PERMISSIONS;
        std::vector<std::string> MANAGED_PERMISSIONS_LIST;

    private:
        PermissionManager() = default;

        mutable std::mutex mutex_;
        std::unordered_map<std::string, std::string> prefix_cache_;
        std::unordered_map<std::string, std::string> suffix_cache_;
        std::unordered_map<std::string, std::map<std::string, bool>> perm_cache_;
    };

} // namespace primebds::permissions
