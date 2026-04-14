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

        /// Returns the rank names from the permissions config in order.
        std::vector<std::string> getRanks() const;

        /// Returns true if rank1 outranks rank2 (lower index = higher rank).
        bool checkInternalRank(const std::string &rank1, const std::string &rank2) const;

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
