/// @file server_db.h
/// Server database for warps, homes, spawns, gamerules, name bans.

#pragma once

#include "primebds/utils/database/database_manager.h"
#include "primebds/utils/database/models.h"

#include <nlohmann/json.hpp>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace primebds::db {

    class ServerDB : public DatabaseManager {
    public:
        explicit ServerDB(const std::string &db_name);

        // Server info
        void updateServerInfo(const std::string &column, const std::string &value);
        ServerData getServerInfo();
        std::map<std::string, int> getGamerules();

        // Name bans
        void addNameBan(const std::string &name, const std::string &reason, int64_t duration);
        void removeNameBan(const std::string &name);
        bool checkNameBan(const std::string &name);
        std::optional<NameBan> getNameBanInfo(const std::string &name);
        std::vector<NameBan> getAllNameBans();

        // Warps
        bool createWarp(const std::string &name, const std::string &pos_json,
                        const std::string &displayname = "", const std::string &category = "",
                        const std::string &description = "", double cost = 0.0,
                        double cooldown = 0.0, double delay = 0.0,
                        const std::vector<std::string> &aliases = {});
        bool deleteWarp(const std::string &name);
        std::optional<Warp> getWarp(const std::string &name);
        std::vector<Warp> getAllWarps();
        bool updateWarpProperty(const std::string &name, const std::string &field,
                                const std::string &value);
        bool addAlias(const std::string &warp_name, const std::string &alias);
        bool removeAlias(const std::string &warp_name, const std::string &alias);

        // Homes
        bool createHome(const std::string &xuid, const std::string &username,
                        const std::string &name, const std::string &pos_json);
        bool deleteHome(const std::string &name, const std::string &username,
                        const std::string &xuid);
        std::optional<Home> getHome(const std::string &name, const std::string &username,
                                    const std::string &xuid);
        std::map<std::string, Home> getAllHomes(const std::string &username,
                                                const std::string &xuid);
        HomeSettings getHomeSettings();
        void setHomeSettings(double delay, double cooldown, double cost);

        // Spawns
        void setSpawn(const std::string &pos_json);
        std::optional<Spawn> getSpawn();

        // Allowlist
        std::vector<std::string> getAllowlistEntries();
        bool addToAllowlist(const std::string &name);
        bool removeFromAllowlist(const std::string &name);
        bool isOnAllowlist(const std::string &name);
        std::vector<AllowlistProfile> getAllowlistProfiles();
        bool createAllowlistProfile(const std::string &name);
        bool deleteAllowlistProfile(const std::string &name);
        bool setActiveAllowlistProfile(const std::string &name);
        bool setAllowlistInheritance(const std::string &profile, const std::string &parent);
        void clearAllowlist();

        // Ranks
        std::vector<Rank> getAllRanks();
        bool createRank(const std::string &name);
        bool deleteRank(const std::string &name);
        std::optional<Rank> getRank(const std::string &name);
        bool addRankPermission(const std::string &rank, const std::string &perm);
        bool removeRankPermission(const std::string &rank, const std::string &perm);
        void setRankInheritance(const std::string &rank, const std::string &parent);
        void setRankWeight(const std::string &rank, int weight);
        void setRankPrefix(const std::string &rank, const std::string &prefix);
        void setRankSuffix(const std::string &rank, const std::string &suffix);

        // Location encoding
        static std::string encodeLocation(double x, double y, double z,
                                          const std::string &dimension,
                                          float pitch, float yaw);
        static nlohmann::json decodeLocation(const std::string &pos_str);

    private:
        void createTables();
    };

} // namespace primebds::db
