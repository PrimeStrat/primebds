/// @file session_db.cpp
/// Session logging database implementation.

#include "primebds/utils/database/session_db.h"

#include <ctime>

namespace primebds::db
{

    SessionDB::SessionDB(const std::string &db_name) : DatabaseManager(db_name)
    {
        createTables();
    }

    void SessionDB::createTables()
    {
        createTable("sessions", {{"id", "INTEGER PRIMARY KEY AUTOINCREMENT"},
                                 {"xuid", "TEXT"},
                                 {"name", "TEXT"},
                                 {"join_time", "INTEGER"},
                                 {"leave_time", "INTEGER DEFAULT 0"},
                                 {"ip_address", "TEXT DEFAULT ''"},
                                 {"device_os", "TEXT DEFAULT ''"}});
    }

    void SessionDB::insertSession(const std::string &xuid, const std::string &name,
                                  const std::string &ip, const std::string &device_os)
    {
        auto now = std::to_string(std::time(nullptr));
        execute(
            "INSERT INTO sessions (xuid, name, join_time, ip_address, device_os) VALUES (?, ?, ?, ?, ?)",
            {xuid, name, now, ip, device_os});
    }

    void SessionDB::endSession(const std::string &xuid)
    {
        auto now = std::to_string(std::time(nullptr));
        execute(
            "UPDATE sessions SET leave_time = ? WHERE xuid = ? AND leave_time = 0",
            {now, xuid});
    }

    std::vector<std::map<std::string, std::string>> SessionDB::getActiveSessions()
    {
        return query("SELECT * FROM sessions WHERE leave_time = 0");
    }

    int64_t SessionDB::getTotalPlaytime(const std::string &xuid)
    {
        auto rows = query(
            "SELECT join_time, leave_time FROM sessions WHERE xuid = ? AND leave_time > 0",
            {xuid});

        int64_t total = 0;
        for (auto &r : rows)
        {
            int64_t join = std::stoll(r["join_time"]);
            int64_t leave = std::stoll(r["leave_time"]);
            total += (leave - join);
        }
        return total;
    }

    std::vector<std::map<std::string, std::string>> SessionDB::getRecentSessions(
        const std::string &xuid, int limit)
    {
        return query(
            "SELECT * FROM sessions WHERE xuid = ? ORDER BY join_time DESC LIMIT ?",
            {xuid, std::to_string(limit)});
    }

} // namespace primebds::db
