/// @file user_db.cpp
/// User database implementation.

#include "primebds/utils/database/user_db.h"

#include <nlohmann/json.hpp>
#include <ctime>

namespace primebds::db {

    UserDB::UserDB(const std::string &db_name) : DatabaseManager(db_name) {
        createTables();
    }

    void UserDB::createTables() {
        createTable("users", {{"xuid", "TEXT UNIQUE NOT NULL"},
                              {"uuid", "TEXT"},
                              {"name", "TEXT"},
                              {"ping", "INTEGER DEFAULT 0"},
                              {"device_os", "TEXT"},
                              {"device_id", "TEXT"},
                              {"unique_id", "INTEGER DEFAULT 0"},
                              {"client_ver", "TEXT"},
                              {"internal_rank", "TEXT DEFAULT 'default'"},
                              {"gamemode", "INTEGER DEFAULT 0"},
                              {"xp", "INTEGER DEFAULT 0"},
                              {"perms", "TEXT DEFAULT '{}'"},
                              {"is_silent_muted", "INTEGER DEFAULT 0"},
                              {"is_afk", "INTEGER DEFAULT 0"},
                              {"last_messaged", "TEXT DEFAULT ''"},
                              {"last_join", "INTEGER DEFAULT 0"},
                              {"last_leave", "INTEGER DEFAULT 0"},
                              {"last_logout_pos", "TEXT DEFAULT ''"},
                              {"last_logout_dim", "TEXT DEFAULT ''"},
                              {"enabled_mt", "INTEGER DEFAULT 0"},
                              {"enabled_ss", "INTEGER DEFAULT 0"},
                              {"enabled_ms", "INTEGER DEFAULT 0"},
                              {"enabled_as", "INTEGER DEFAULT 0"},
                              {"enabled_sc", "INTEGER DEFAULT 0"}});

        createTable("mod_logs", {{"xuid", "TEXT UNIQUE NOT NULL"},
                                 {"name", "TEXT"},
                                 {"is_muted", "INTEGER DEFAULT 0"},
                                 {"mute_time", "INTEGER DEFAULT 0"},
                                 {"mute_reason", "TEXT DEFAULT ''"},
                                 {"is_banned", "INTEGER DEFAULT 0"},
                                 {"banned_time", "INTEGER DEFAULT 0"},
                                 {"ban_reason", "TEXT DEFAULT ''"},
                                 {"ip_address", "TEXT DEFAULT ''"},
                                 {"is_ip_banned", "INTEGER DEFAULT 0"},
                                 {"is_ip_muted", "INTEGER DEFAULT 0"}});

        createTable("warnings", {{"id", "INTEGER PRIMARY KEY AUTOINCREMENT"},
                                 {"xuid", "TEXT"},
                                 {"name", "TEXT"},
                                 {"warn_reason", "TEXT"},
                                 {"warn_time", "INTEGER"},
                                 {"added_by", "TEXT"}});

        createTable("notes", {{"id", "INTEGER PRIMARY KEY AUTOINCREMENT"},
                              {"xuid", "TEXT"},
                              {"name", "TEXT"},
                              {"note", "TEXT"},
                              {"timestamp", "INTEGER"},
                              {"added_by", "TEXT"}});

        createTable("punishment_log", {{"id", "INTEGER PRIMARY KEY AUTOINCREMENT"},
                                       {"xuid", "TEXT"},
                                       {"name", "TEXT"},
                                       {"action_type", "TEXT"},
                                       {"reason", "TEXT"},
                                       {"timestamp", "INTEGER"},
                                       {"duration", "INTEGER"}});
    }

    void UserDB::saveUser(const std::string &xuid, const std::string &uuid,
                          const std::string &name, int ping,
                          const std::string &device_os, const std::string &device_id,
                          int64_t unique_id, const std::string &client_ver) {
        auto existing = queryRow("SELECT xuid FROM users WHERE xuid = ?", {xuid});
        auto now = std::to_string(std::time(nullptr));

        if (existing) {
            execute(
                "UPDATE users SET uuid=?, name=?, ping=?, device_os=?, device_id=?, "
                "unique_id=?, client_ver=?, last_join=? WHERE xuid=?",
                {uuid, name, std::to_string(ping), device_os, device_id,
                 std::to_string(unique_id), client_ver, now, xuid});
        } else {
            execute(
                "INSERT INTO users (xuid, uuid, name, ping, device_os, device_id, "
                "unique_id, client_ver, internal_rank, last_join) "
                "VALUES (?, ?, ?, ?, ?, ?, ?, ?, 'default', ?)",
                {xuid, uuid, name, std::to_string(ping), device_os, device_id,
                 std::to_string(unique_id), client_ver, now});
        }

        ensureModLog(xuid, name);
        invalidateUserCache(xuid);
    }

    std::optional<User> UserDB::getOnlineUser(const std::string &xuid) { {
            std::lock_guard lock(cache_mutex_);
            auto it = user_cache_.find(xuid);
            if (it != user_cache_.end()) {
                auto elapsed = std::chrono::steady_clock::now() - it->second.second;
                if (elapsed < CACHE_TTL) {
                    return it->second.first;
                }
            }
        }

        auto row = queryRow("SELECT * FROM users WHERE xuid = ?", {xuid});
        if (!row)
            return std::nullopt;

        User u;
        auto &r = *row;
        u.xuid = r["xuid"];
        u.uuid = r["uuid"];
        u.name = r["name"];
        u.ping = std::stoi(r["ping"]);
        u.device_os = r["device_os"];
        u.device_id = r["device_id"];
        u.unique_id = std::stoll(r["unique_id"]);
        u.client_ver = r["client_ver"];
        u.internal_rank = r["internal_rank"];
        u.gamemode = std::stoi(r["gamemode"]);
        u.xp = std::stoi(r["xp"]);
        u.perms = r["perms"];
        u.is_silent_muted = std::stoi(r["is_silent_muted"]);
        u.is_afk = std::stoi(r["is_afk"]);
        u.last_messaged = r["last_messaged"];
        u.last_join = std::stoll(r["last_join"]);
        u.last_leave = std::stoll(r["last_leave"]);
        u.last_logout_pos = r["last_logout_pos"];
        u.last_logout_dim = r["last_logout_dim"];
        u.enabled_mt = std::stoi(r["enabled_mt"]);
        u.enabled_ss = std::stoi(r["enabled_ss"]);
        u.enabled_ms = std::stoi(r["enabled_ms"]);
        u.enabled_as = std::stoi(r["enabled_as"]);
        u.enabled_sc = std::stoi(r["enabled_sc"]);

        {
            std::lock_guard lock(cache_mutex_);
            user_cache_[xuid] = {u, std::chrono::steady_clock::now()};
        }

        return u;
    }

    std::optional<User> UserDB::getUserByName(const std::string &name) {
        // Check cache first (search by name across cached users)
        {
            std::lock_guard lock(cache_mutex_);
            auto now = std::chrono::steady_clock::now();
            for (auto &[xuid, entry] : user_cache_) {
                if (now - entry.second >= CACHE_TTL)
                    continue;
                // Case-insensitive name comparison
                const auto &cached_name = entry.first.name;
                if (cached_name.size() == name.size() &&
                    std::equal(cached_name.begin(), cached_name.end(), name.begin(),
                               [](char a, char b) { return std::tolower(a) == std::tolower(b); })) {
                    return entry.first;
                }
            }
        }

        auto row = queryRow("SELECT * FROM users WHERE name = ? COLLATE NOCASE", {name});
        if (!row)
            return std::nullopt;

        User u;
        auto &r = *row;
        u.xuid = r["xuid"];
        u.uuid = r["uuid"];
        u.name = r["name"];
        u.ping = std::stoi(r["ping"]);
        u.device_os = r["device_os"];
        u.device_id = r["device_id"];
        u.unique_id = std::stoll(r["unique_id"]);
        u.client_ver = r["client_ver"];
        u.internal_rank = r["internal_rank"];
        u.gamemode = std::stoi(r["gamemode"]);
        u.xp = std::stoi(r["xp"]);
        u.perms = r["perms"];
        u.is_silent_muted = std::stoi(r["is_silent_muted"]);
        u.is_afk = std::stoi(r["is_afk"]);
        u.last_messaged = r["last_messaged"];
        u.last_join = std::stoll(r["last_join"]);
        u.last_leave = std::stoll(r["last_leave"]);
        u.last_logout_pos = r["last_logout_pos"];
        u.last_logout_dim = r["last_logout_dim"];
        u.enabled_mt = std::stoi(r["enabled_mt"]);
        u.enabled_ss = std::stoi(r["enabled_ss"]);
        u.enabled_ms = std::stoi(r["enabled_ms"]);
        u.enabled_as = std::stoi(r["enabled_as"]);
        u.enabled_sc = std::stoi(r["enabled_sc"]);

        // Cache the result by xuid for subsequent lookups
        {
            std::lock_guard lock(cache_mutex_);
            user_cache_[u.xuid] = {u, std::chrono::steady_clock::now()};
        }
        return u;
    }

    void UserDB::updateUser(const std::string &xuid, const std::string &column,
                            const std::string &value) {
        execute("UPDATE users SET " + column + " = ? WHERE xuid = ?", {value, xuid});
        invalidateUserCache(xuid);
    }

    // --- Mod log ---

    void UserDB::ensureModLog(const std::string &xuid, const std::string &name) {
        auto existing = queryRow("SELECT xuid FROM mod_logs WHERE xuid = ?", {xuid});
        if (!existing) {
            execute("INSERT INTO mod_logs (xuid, name) VALUES (?, ?)", {xuid, name});
        } else {
            execute("UPDATE mod_logs SET name = ? WHERE xuid = ?", {name, xuid});
        }
    }

    std::optional<ModLog> UserDB::getModLog(const std::string &xuid) { {
            std::lock_guard lock(cache_mutex_);
            auto it = modlog_cache_.find(xuid);
            if (it != modlog_cache_.end()) {
                auto elapsed = std::chrono::steady_clock::now() - it->second.second;
                if (elapsed < CACHE_TTL)
                    return it->second.first;
            }
        }

        auto row = queryRow("SELECT * FROM mod_logs WHERE xuid = ?", {xuid});
        if (!row)
            return std::nullopt;

        ModLog m;
        auto &r = *row;
        m.xuid = r["xuid"];
        m.name = r["name"];
        m.is_muted = r["is_muted"] == "1";
        m.mute_time = std::stoll(r["mute_time"]);
        m.mute_reason = r["mute_reason"];
        m.is_banned = r["is_banned"] == "1";
        m.banned_time = std::stoll(r["banned_time"]);
        m.ban_reason = r["ban_reason"];
        m.ip_address = r["ip_address"];
        m.is_ip_banned = r["is_ip_banned"] == "1";
        m.is_ip_muted = r["is_ip_muted"] == "1";

        {
            std::lock_guard lock(cache_mutex_);
            modlog_cache_[xuid] = {m, std::chrono::steady_clock::now()};
        }

        return m;
    }

    void UserDB::updateModLog(const std::string &xuid, const std::string &column,
                              const std::string &value) {
        execute("UPDATE mod_logs SET " + column + " = ? WHERE xuid = ?", {value, xuid});
        invalidateModLogCache(xuid);
    }

    // --- Ban/mute ---

    bool UserDB::checkBan(const std::string &xuid) {
        auto mod = getModLog(xuid);
        if (!mod || !mod->is_banned)
            return false;

        if (mod->banned_time > 0 && std::time(nullptr) >= mod->banned_time) {
            // Ban expired
            updateModLog(xuid, "is_banned", "0");
            updateModLog(xuid, "banned_time", "0");
            updateModLog(xuid, "ban_reason", "");
            return false;
        }
        return true;
    }

    bool UserDB::checkAndUpdateMute(const std::string &xuid, const std::string &name) {
        auto mod = getModLog(xuid);
        if (!mod || !mod->is_muted)
            return false;

        if (mod->mute_time > 0 && std::time(nullptr) >= mod->mute_time) {
            updateModLog(xuid, "is_muted", "0");
            updateModLog(xuid, "mute_time", "0");
            updateModLog(xuid, "mute_reason", "");
            return false;
        }
        return true;
    }

    std::tuple<bool, int64_t, std::string> UserDB::checkIpMute(const std::string &ip) {
        auto rows = query(
            "SELECT is_ip_muted, mute_time, mute_reason, ip_address FROM mod_logs WHERE is_ip_muted = '1'");
        for (auto &r : rows) {
            if (r["ip_address"].find(ip) != std::string::npos) {
                int64_t mute_time = std::stoll(r["mute_time"]);
                if (mute_time > 0 && std::time(nullptr) >= mute_time)
                    continue;
                return {true, mute_time, r["mute_reason"]};
            }
        }
        return {false, 0, ""};
    }

    bool UserDB::checkIpBan(const std::string &ip) {
        auto rows = query("SELECT is_ip_banned, ip_address, banned_time FROM mod_logs WHERE is_ip_banned = '1'");
        for (auto &r : rows) {
            if (r["ip_address"].find(ip) != std::string::npos) {
                int64_t banned_time = std::stoll(r["banned_time"]);
                if (banned_time > 0 && std::time(nullptr) >= banned_time)
                    continue;
                return true;
            }
        }
        return false;
    }

    std::vector<ModLog> UserDB::getAllBanned() {
        auto rows = query("SELECT * FROM mod_logs WHERE is_banned = '1'");
        std::vector<ModLog> result;
        for (auto &r : rows) {
            ModLog m;
            m.xuid = r["xuid"];
            m.name = r["name"];
            m.is_banned = true;
            m.banned_time = std::stoll(r["banned_time"]);
            m.ban_reason = r["ban_reason"];
            result.push_back(m);
        }
        return result;
    }

    std::vector<ModLog> UserDB::getAllMuted() {
        auto rows = query("SELECT * FROM mod_logs WHERE is_muted = '1'");
        std::vector<ModLog> result;
        for (auto &r : rows) {
            ModLog m;
            m.xuid = r["xuid"];
            m.name = r["name"];
            m.is_muted = true;
            m.mute_time = std::stoll(r["mute_time"]);
            m.mute_reason = r["mute_reason"];
            result.push_back(m);
        }
        return result;
    }

    // --- Warnings ---

    void UserDB::addWarning(const std::string &xuid, const std::string &name,
                            const std::string &reason, const std::string &added_by) {
        auto now = std::to_string(std::time(nullptr));
        execute(
            "INSERT INTO warnings (xuid, name, warn_reason, warn_time, added_by) VALUES (?, ?, ?, ?, ?)",
            {xuid, name, reason, now, added_by});
    }

    void UserDB::removeWarning(int id) {
        execute("DELETE FROM warnings WHERE id = ?", {std::to_string(id)});
    }

    std::vector<Warn> UserDB::getWarnings(const std::string &xuid) {
        auto rows = query("SELECT * FROM warnings WHERE xuid = ?", {xuid});
        std::vector<Warn> warns;
        for (auto &r : rows) {
            Warn w;
            w.id = std::stoi(r["id"]);
            w.xuid = r["xuid"];
            w.name = r["name"];
            w.warn_reason = r["warn_reason"];
            w.warn_time = std::stoll(r["warn_time"]);
            w.added_by = r["added_by"];
            warns.push_back(w);
        }
        return warns;
    }

    int UserDB::getWarningCount(const std::string &xuid) {
        auto row = queryRow("SELECT COUNT(*) as cnt FROM warnings WHERE xuid = ?", {xuid});
        if (row)
            return std::stoi((*row)["cnt"]);
        return 0;
    }

    // --- Notes ---

    void UserDB::addNote(const std::string &xuid, const std::string &name,
                         const std::string &note, const std::string &added_by) {
        auto now = std::to_string(std::time(nullptr));
        execute(
            "INSERT INTO notes (xuid, name, note, timestamp, added_by) VALUES (?, ?, ?, ?, ?)",
            {xuid, name, note, now, added_by});
    }

    void UserDB::removeNote(int id) {
        execute("DELETE FROM notes WHERE id = ?", {std::to_string(id)});
    }

    std::vector<Note> UserDB::getNotes(const std::string &xuid) {
        auto rows = query("SELECT * FROM notes WHERE xuid = ?", {xuid});
        std::vector<Note> notes;
        for (auto &r : rows) {
            Note n;
            n.id = std::stoi(r["id"]);
            n.xuid = r["xuid"];
            n.name = r["name"];
            n.note = r["note"];
            n.timestamp = std::stoll(r["timestamp"]);
            n.added_by = r["added_by"];
            notes.push_back(n);
        }
        return notes;
    }

    // --- Punishment log ---

    void UserDB::logPunishment(const std::string &xuid, const std::string &name,
                               const std::string &action_type, const std::string &reason,
                               std::optional<int64_t> duration) {
        auto now = std::to_string(std::time(nullptr));
        std::string dur = duration ? std::to_string(*duration) : "";
        execute(
            "INSERT INTO punishment_log (xuid, name, action_type, reason, timestamp, duration) "
            "VALUES (?, ?, ?, ?, ?, ?)",
            {xuid, name, action_type, reason, now, dur});
    }

    std::vector<PunishmentLog> UserDB::getPunishmentHistory(const std::string &xuid) {
        auto rows = query("SELECT * FROM punishment_log WHERE xuid = ? ORDER BY timestamp DESC", {xuid});
        std::vector<PunishmentLog> logs;
        for (auto &r : rows) {
            PunishmentLog p;
            p.id = std::stoi(r["id"]);
            p.xuid = r["xuid"];
            p.name = r["name"];
            p.action_type = r["action_type"];
            p.reason = r["reason"];
            p.timestamp = std::stoll(r["timestamp"]);
            if (!r["duration"].empty())
                p.duration = std::stoll(r["duration"]);
            logs.push_back(p);
        }
        return logs;
    }

    // --- Permissions (stored as JSON in users.perms column, mirroring Python) ---

    std::map<std::string, bool> UserDB::getPermissions(const std::string &xuid) {
        std::map<std::string, bool> perms;
        auto user = getOnlineUser(xuid);
        if (!user || user->perms.empty())
            return perms;
        try {
            auto j = nlohmann::json::parse(user->perms);
            if (!j.is_object())
                return perms;
            for (auto &[k, v] : j.items()) {
                if (v.is_boolean())
                    perms[k] = v.get<bool>();
                else if (v.is_number())
                    perms[k] = v.get<int>() != 0;
            }
        } catch (...) {
        }
        return perms;
    }

    void UserDB::setPermission(const std::string &xuid, const std::string &perm, bool value) {
        nlohmann::json j = nlohmann::json::object();
        auto user = getOnlineUser(xuid);
        if (user && !user->perms.empty()) {
            try {
                auto parsed = nlohmann::json::parse(user->perms);
                if (parsed.is_object())
                    j = parsed;
            } catch (...) {
            }
        }
        j[perm] = value;
        updateUser(xuid, "perms", j.dump());
    }

    void UserDB::removePermission(const std::string &xuid, const std::string &perm) {
        auto user = getOnlineUser(xuid);
        if (!user || user->perms.empty())
            return;
        try {
            auto j = nlohmann::json::parse(user->perms);
            if (!j.is_object())
                return;
            j.erase(perm);
            updateUser(xuid, "perms", j.dump());
        } catch (...) {
        }
    }

    // --- Alt detection ---

    std::vector<Alt> UserDB::findAlts(const std::string &ip, const std::string &device_id,
                                      const std::string &exclude_xuid) {
        auto rows = query(
            "SELECT u.name, u.xuid, m.ip_address, u.device_id FROM users u "
            "LEFT JOIN mod_logs m ON u.xuid = m.xuid "
            "WHERE u.xuid != ? AND (m.ip_address LIKE ? OR u.device_id = ?)",
            {exclude_xuid, "%" + ip + "%", device_id});

        std::vector<Alt> alts;
        for (auto &r : rows) {
            Alt a;
            a.alt_name = r["name"];
            a.alt_xuid = r["xuid"];
            alts.push_back(a);
        }
        return alts;
    }

    // --- Cache ---

    void UserDB::invalidateUserCache(const std::string &xuid) {
        std::lock_guard lock(cache_mutex_);
        user_cache_.erase(xuid);
    }

    void UserDB::invalidateModLogCache(const std::string &xuid) {
        std::lock_guard lock(cache_mutex_);
        modlog_cache_.erase(xuid);
    }

    // ---------------------------------------------------------------------------
    // Extended query operations
    // ---------------------------------------------------------------------------

    std::vector<User> UserDB::getAllUsers() {
        auto rows = query("SELECT * FROM users");
        std::vector<User> result;
        for (auto &r : rows) {
            User u;
            u.xuid = r["xuid"];
            u.name = r["name"];
            u.internal_rank = r.count("internal_rank") ? r.at("internal_rank") : "";
            result.push_back(u);
        }
        return result;
    }

    std::vector<ModLog> UserDB::getMutedUsers() {
        auto rows = query("SELECT * FROM mod_logs WHERE is_muted = 1");
        std::vector<ModLog> result;
        for (auto &r : rows) {
            ModLog m;
            m.xuid = r["xuid"];
            m.name = r["name"];
            m.is_muted = true;
            m.mute_time = r.count("mute_time") ? std::stoll(r.at("mute_time")) : 0;
            m.mute_reason = r.count("mute_reason") ? r.at("mute_reason") : "";
            result.push_back(m);
        }
        return result;
    }

    std::vector<ModLog> UserDB::getBannedUsers() {
        auto rows = query("SELECT * FROM mod_logs WHERE is_banned = 1");
        std::vector<ModLog> result;
        for (auto &r : rows) {
            ModLog m;
            m.xuid = r["xuid"];
            m.name = r["name"];
            m.is_banned = true;
            m.banned_time = r.count("banned_time") ? std::stoll(r.at("banned_time")) : 0;
            m.ban_reason = r.count("ban_reason") ? r.at("ban_reason") : "";
            result.push_back(m);
        }
        return result;
    }

    std::vector<ModLog> UserDB::getIPBannedUsers() {
        auto rows = query("SELECT * FROM mod_logs WHERE is_ip_banned = 1");
        std::vector<ModLog> result;
        for (auto &r : rows) {
            ModLog m;
            m.xuid = r["xuid"];
            m.name = r["name"];
            m.is_ip_banned = true;
            m.ip_address = r.count("ip_address") ? r.at("ip_address") : "";
            result.push_back(m);
        }
        return result;
    }

    std::map<std::string, std::map<std::string, std::string>> UserDB::getAllInternalPermissions() {
        // Per-user perms now live in users.perms JSON column (mirrors Python).
        std::map<std::string, std::map<std::string, std::string>> result;
        auto rows = query("SELECT xuid, perms FROM users WHERE perms IS NOT NULL AND perms != ''");
        for (auto &r : rows) {
            try {
                auto j = nlohmann::json::parse(r["perms"]);
                if (!j.is_object())
                    continue;
                for (auto &[k, v] : j.items()) {
                    std::string s;
                    if (v.is_boolean())
                        s = v.get<bool>() ? "1" : "0";
                    else
                        s = v.dump();
                    result[r["xuid"]][k] = s;
                }
            } catch (...) {
            }
        }
        return result;
    }

    std::optional<User> UserDB::getUserByXuid(const std::string &xuid) {
        return getOnlineUser(xuid);
    }

    std::map<std::string, std::string> UserDB::getInternalPermissions(const std::string &xuid) {
        // Per-user perms now live in users.perms JSON column (mirrors Python).
        std::map<std::string, std::string> result;
        auto user = getOnlineUser(xuid);
        if (!user || user->perms.empty())
            return result;
        try {
            auto j = nlohmann::json::parse(user->perms);
            if (!j.is_object())
                return result;
            for (auto &[k, v] : j.items()) {
                if (v.is_boolean())
                    result[k] = v.get<bool>() ? "1" : "0";
                else
                    result[k] = v.dump();
            }
        } catch (...) {
        }
        return result;
    }

    void UserDB::setUserRank(const std::string &xuid, const std::string &rank) {
        updateUser(xuid, "internal_rank", rank);
        invalidateUserCache(xuid);
    }

} // namespace primebds::db
