/// @file user_db.h
/// User database for player data, mod logs, warnings, notes, punishments.

#pragma once

#include "primebds/utils/database/database_manager.h"
#include "primebds/utils/database/models.h"

#include <map>
#include <optional>
#include <set>
#include <string>
#include <tuple>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <mutex>

namespace primebds::db
{

    class UserDB : public DatabaseManager
    {
    public:
        explicit UserDB(const std::string &db_name);

        // User operations
        void saveUser(const std::string &xuid, const std::string &uuid, const std::string &name,
                      int ping, const std::string &device_os, const std::string &device_id,
                      int64_t unique_id, const std::string &client_ver);
        std::optional<User> getOnlineUser(const std::string &xuid);
        std::optional<User> getUserByName(const std::string &name);
        void updateUser(const std::string &xuid, const std::string &column, const std::string &value);

        // Mod log operations
        std::optional<ModLog> getModLog(const std::string &xuid);
        void updateModLog(const std::string &xuid, const std::string &column, const std::string &value);
        void ensureModLog(const std::string &xuid, const std::string &name);

        // Ban operations
        bool checkBan(const std::string &xuid);
        bool checkAndUpdateMute(const std::string &xuid, const std::string &name);
        std::tuple<bool, int64_t, std::string> checkIpMute(const std::string &ip);
        bool checkIpBan(const std::string &ip);
        std::vector<ModLog> getAllBanned();
        std::vector<ModLog> getAllMuted();

        // Warning operations
        void addWarning(const std::string &xuid, const std::string &name,
                        const std::string &reason, const std::string &added_by);
        void removeWarning(int id);
        std::vector<Warn> getWarnings(const std::string &xuid);
        int getWarningCount(const std::string &xuid);

        // Note operations
        void addNote(const std::string &xuid, const std::string &name,
                     const std::string &note, const std::string &added_by);
        void removeNote(int id);
        std::vector<Note> getNotes(const std::string &xuid);

        // Punishment log
        void logPunishment(const std::string &xuid, const std::string &name,
                           const std::string &action_type, const std::string &reason,
                           std::optional<int64_t> duration = std::nullopt);
        std::vector<PunishmentLog> getPunishmentHistory(const std::string &xuid);

        // Permission operations
        std::map<std::string, bool> getPermissions(const std::string &xuid);
        void setPermission(const std::string &xuid, const std::string &perm, bool value);
        void removePermission(const std::string &xuid, const std::string &perm);

        // Extended query operations
        std::vector<User> getAllUsers();
        std::vector<ModLog> getMutedUsers();
        std::vector<ModLog> getBannedUsers();
        std::vector<ModLog> getIPBannedUsers();
        std::map<std::string, std::map<std::string, std::string>> getAllInternalPermissions();
        std::optional<User> getUserByXuid(const std::string &xuid);
        std::map<std::string, std::string> getInternalPermissions(const std::string &xuid);
        void setUserRank(const std::string &xuid, const std::string &rank);

        // Alt detection
        std::vector<Alt> findAlts(const std::string &ip, const std::string &device_id,
                                  const std::string &exclude_xuid);

        // Cache management
        void invalidateUserCache(const std::string &xuid);
        void invalidateModLogCache(const std::string &xuid);

    private:
        void createTables();

        // Cache
        mutable std::mutex cache_mutex_;
        std::unordered_map<std::string, std::pair<User, std::chrono::steady_clock::time_point>> user_cache_;
        std::unordered_map<std::string, std::pair<ModLog, std::chrono::steady_clock::time_point>> modlog_cache_;
        static constexpr auto CACHE_TTL = std::chrono::seconds(60);
    };

} // namespace primebds::db
