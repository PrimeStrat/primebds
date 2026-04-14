/// @file session_db.h
/// Session logging database for tracking player sessions.

#pragma once

#include "primebds/utils/database/database_manager.h"

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace primebds::db {

    class SessionDB : public DatabaseManager {
    public:
        explicit SessionDB(const std::string &db_name);

        void insertSession(const std::string &xuid, const std::string &name,
                           const std::string &ip, const std::string &device_os);
        void endSession(const std::string &xuid);

        std::vector<std::map<std::string, std::string>> getActiveSessions();
        std::vector<std::map<std::string, std::string>> getRecentSessions(
            const std::string &xuid, int limit = 10);
        int64_t getTotalPlaytime(const std::string &xuid);

    private:
        void createTables();
    };

} // namespace primebds::db
