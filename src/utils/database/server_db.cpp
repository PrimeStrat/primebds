/// @file server_db.cpp
/// Server database implementation.

#include "primebds/utils/database/server_db.h"

#include <ctime>
#include <iostream>
#include <set>

namespace primebds::db {

    ServerDB::ServerDB(const std::string &db_name) : DatabaseManager(db_name) {
        createTables();
    }

    void ServerDB::createTables() {
        createTable("server_info", {{"id", "INTEGER PRIMARY KEY CHECK (id = 1)"},
                                    {"last_shutdown_time", "INTEGER"},
                                    {"allowlist_profile", "TEXT"},
                                    {"can_interact", "INTEGER DEFAULT 1"},
                                    {"can_emote", "INTEGER DEFAULT 1"},
                                    {"can_decay_leaves", "INTEGER DEFAULT 1"},
                                    {"can_change_skin", "INTEGER DEFAULT 1"},
                                    {"can_pickup_items", "INTEGER DEFAULT 1"},
                                    {"can_sleep", "INTEGER DEFAULT 1"},
                                    {"can_split_screen", "INTEGER DEFAULT 1"}});

        createTable("name_bans", {{"name", "TEXT UNIQUE NOT NULL"},
                                  {"banned_time", "INTEGER"},
                                  {"ban_reason", "TEXT"}});

        createTable("warps", {{"name", "TEXT UNIQUE NOT NULL"},
                              {"pos", "TEXT"},
                              {"displayname", "TEXT"},
                              {"category", "TEXT"},
                              {"description", "TEXT"},
                              {"cost", "REAL DEFAULT 0"},
                              {"cooldown", "REAL DEFAULT 0"},
                              {"delay", "REAL DEFAULT 0"},
                              {"aliases", "TEXT"}});

        createTable("homes", {{"xuid", "TEXT"},
                              {"username", "TEXT"},
                              {"name", "TEXT"},
                              {"pos", "TEXT"},
                              {"cooldown", "REAL DEFAULT 0"},
                              {"delay", "REAL DEFAULT 0"},
                              {"cost", "REAL DEFAULT 0"},
                              {"UNIQUE (xuid, name)", ""},
                              {"UNIQUE (username, name)", ""}});

        createTable("spawns", {{"id", "INTEGER PRIMARY KEY CHECK (id = 1)"},
                               {"pos", "TEXT"},
                               {"cost", "REAL DEFAULT 0"},
                               {"cooldown", "REAL DEFAULT 0"},
                               {"delay", "REAL DEFAULT 0"}});

        createTable("last_warp", {{"xuid", "TEXT"},
                                  {"username", "TEXT"},
                                  {"name", "TEXT"},
                                  {"pos", "TEXT"},
                                  {"cooldown", "REAL DEFAULT 0"},
                                  {"delay", "REAL DEFAULT 0"},
                                  {"cost", "REAL DEFAULT 0"},
                                  {"UNIQUE (xuid)", ""},
                                  {"UNIQUE (username)", ""}});

        createTable("home_settings", {{"id", "INTEGER PRIMARY KEY CHECK (id = 1)"},
                                      {"delay", "REAL DEFAULT 0"},
                                      {"cooldown", "REAL DEFAULT 0"},
                                      {"cost", "REAL DEFAULT 0"}});

        // Insert defaults
        try {
            execute("INSERT OR IGNORE INTO server_info (id, last_shutdown_time) VALUES ('1', '0')");
            execute("INSERT OR IGNORE INTO home_settings (id, delay, cooldown, cost) VALUES ('1', '0', '0', '0')");
        } catch (...) {
        }
    }

    void ServerDB::updateServerInfo(const std::string &column, const std::string &value) {
        execute("UPDATE server_info SET " + column + " = ? WHERE id = 1", {value});
    }

    ServerData ServerDB::getServerInfo() {
        auto row = queryRow("SELECT * FROM server_info WHERE id = 1");
        ServerData data;
        if (row) {
            auto &r = *row;
            data.last_shutdown_time = std::stoll(r["last_shutdown_time"]);
            data.allowlist_profile = r["allowlist_profile"];
            data.can_interact = std::stoi(r.count("can_interact") ? r["can_interact"] : "1");
            data.can_emote = std::stoi(r.count("can_emote") ? r["can_emote"] : "1");
            data.can_decay_leaves = std::stoi(r.count("can_decay_leaves") ? r["can_decay_leaves"] : "1");
            data.can_change_skin = std::stoi(r.count("can_change_skin") ? r["can_change_skin"] : "1");
            data.can_pickup_items = std::stoi(r.count("can_pickup_items") ? r["can_pickup_items"] : "1");
            data.can_sleep = std::stoi(r.count("can_sleep") ? r["can_sleep"] : "1");
            data.can_split_screen = std::stoi(r.count("can_split_screen") ? r["can_split_screen"] : "1");
        }
        return data;
    }

    std::map<std::string, int> ServerDB::getGamerules() {
        auto row = queryRow(
            "SELECT can_interact, can_emote, can_decay_leaves, can_change_skin, "
            "can_pickup_items, can_sleep, can_split_screen FROM server_info WHERE id = 1");

        std::map<std::string, int> rules;
        if (row) {
            for (auto &[key, val] : *row) {
                rules[key] = std::stoi(val);
            }
        }
        return rules;
    }

    // --- Name bans ---

    void ServerDB::addNameBan(const std::string &name, const std::string &reason, int64_t duration) {
        int64_t banned_until = duration > 0 ? static_cast<int64_t>(std::time(nullptr)) + duration : 0;
        execute("INSERT OR REPLACE INTO name_bans (name, banned_time, ban_reason) VALUES (?, ?, ?)",
                {name, std::to_string(banned_until), reason});
    }

    void ServerDB::removeNameBan(const std::string &name) {
        execute("DELETE FROM name_bans WHERE name = ?", {name});
    }

    bool ServerDB::checkNameBan(const std::string &name) {
        auto row = queryRow("SELECT banned_time FROM name_bans WHERE name = ?", {name});
        if (!row)
            return false;
        return std::time(nullptr) < std::stoll((*row)["banned_time"]);
    }

    std::optional<NameBan> ServerDB::getNameBanInfo(const std::string &name) {
        auto row = queryRow("SELECT * FROM name_bans WHERE name = ?", {name});
        if (!row)
            return std::nullopt;

        NameBan ban;
        ban.name = (*row)["name"];
        ban.banned_time = std::stoll((*row)["banned_time"]);
        ban.ban_reason = (*row)["ban_reason"];
        return ban;
    }

    std::vector<NameBan> ServerDB::getAllNameBans() {
        auto rows = query("SELECT * FROM name_bans");
        std::vector<NameBan> bans;
        for (auto &r : rows) {
            NameBan ban;
            ban.name = r["name"];
            ban.banned_time = std::stoll(r["banned_time"]);
            ban.ban_reason = r["ban_reason"];
            bans.push_back(ban);
        }
        return bans;
    }

    // --- Warps ---

    bool ServerDB::createWarp(const std::string &name, const std::string &pos_json,
                              const std::string &displayname, const std::string &category,
                              const std::string &description, double cost,
                              double cooldown, double delay, const std::vector<std::string> &aliases) {
        auto existing = queryRow("SELECT 1 FROM warps WHERE name = ? COLLATE NOCASE", {name});
        if (existing)
            return false;

        nlohmann::json aliases_json = aliases;
        execute(
            "INSERT INTO warps (name, pos, displayname, category, description, cost, cooldown, delay, aliases) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)",
            {name, pos_json, displayname, category, description, std::to_string(cost),
             std::to_string(cooldown), std::to_string(delay), aliases_json.dump()});
        return true;
    }

    bool ServerDB::deleteWarp(const std::string &name) {
        auto existing = queryRow("SELECT 1 FROM warps WHERE name = ? COLLATE NOCASE", {name});
        if (!existing)
            return false;
        execute("DELETE FROM warps WHERE name = ? COLLATE NOCASE", {name});
        return true;
    }

    std::optional<Warp> ServerDB::getWarp(const std::string &name) {
        auto row = queryRow("SELECT * FROM warps WHERE name = ? COLLATE NOCASE", {name});
        if (!row)
            return std::nullopt;

        Warp w;
        w.name = (*row)["name"];
        w.pos = (*row)["pos"];
        w.displayname = (*row)["displayname"];
        w.category = (*row)["category"];
        w.description = (*row)["description"];
        w.cost = std::stod((*row)["cost"]);
        w.cooldown = std::stod((*row)["cooldown"]);
        w.delay = std::stod((*row)["delay"]);

        try {
            auto aliases_str = (*row)["aliases"];
            if (!aliases_str.empty()) {
                auto j = nlohmann::json::parse(aliases_str);
                w.aliases = j.get<std::vector<std::string>>();
            }
        } catch (...) {
        }

        return w;
    }

    std::vector<Warp> ServerDB::getAllWarps() {
        auto rows = query("SELECT * FROM warps");
        std::vector<Warp> warps;
        for (auto &r : rows) {
            Warp w;
            w.name = r["name"];
            w.pos = r["pos"];
            w.displayname = r["displayname"];
            w.category = r["category"];
            w.description = r["description"];
            w.cost = std::stod(r["cost"]);
            w.cooldown = std::stod(r["cooldown"]);
            w.delay = std::stod(r["delay"]);
            try {
                if (!r["aliases"].empty()) {
                    w.aliases = nlohmann::json::parse(r["aliases"]).get<std::vector<std::string>>();
                }
            } catch (...) {
            }
            warps.push_back(w);
        }
        return warps;
    }

    bool ServerDB::updateWarpProperty(const std::string &name, const std::string &field,
                                      const std::string &value) {
        static const std::set<std::string> allowed = {"pos", "displayname", "category",
                                                      "description", "cost", "cooldown",
                                                      "delay", "aliases"};
        if (allowed.find(field) == allowed.end())
            return false;

        auto existing = queryRow("SELECT 1 FROM warps WHERE name = ? COLLATE NOCASE", {name});
        if (!existing)
            return false;

        execute("UPDATE warps SET " + field + " = ? WHERE name = ? COLLATE NOCASE", {value, name});
        return true;
    }

    bool ServerDB::addAlias(const std::string &warp_name, const std::string &alias) {
        auto row =
            queryRow("SELECT aliases FROM warps WHERE name = ? COLLATE NOCASE", {warp_name});
        if (!row)
            return false;

        std::vector<std::string> aliases;
        try {
            auto &s = (*row)["aliases"];
            if (!s.empty())
                aliases = nlohmann::json::parse(s).get<std::vector<std::string>>();
        } catch (...) {
        }

        std::string lower_alias = alias;
        std::transform(lower_alias.begin(), lower_alias.end(), lower_alias.begin(), ::tolower);

        if (std::find(aliases.begin(), aliases.end(), lower_alias) != aliases.end())
            return false;

        aliases.push_back(lower_alias);
        nlohmann::json j = aliases;
        execute("UPDATE warps SET aliases = ? WHERE name = ? COLLATE NOCASE",
                {j.dump(), warp_name});
        return true;
    }

    bool ServerDB::removeAlias(const std::string &warp_name, const std::string &alias) {
        auto row =
            queryRow("SELECT aliases FROM warps WHERE name = ? COLLATE NOCASE", {warp_name});
        if (!row)
            return false;

        std::vector<std::string> aliases;
        try {
            auto &s = (*row)["aliases"];
            if (!s.empty())
                aliases = nlohmann::json::parse(s).get<std::vector<std::string>>();
        } catch (...) {
        }

        std::string lower_alias = alias;
        std::transform(lower_alias.begin(), lower_alias.end(), lower_alias.begin(), ::tolower);

        auto it = std::find(aliases.begin(), aliases.end(), lower_alias);
        if (it == aliases.end())
            return false;

        aliases.erase(it);
        nlohmann::json j = aliases;
        execute("UPDATE warps SET aliases = ? WHERE name = ? COLLATE NOCASE",
                {j.dump(), warp_name});
        return true;
    }

    // --- Homes ---

    bool ServerDB::createHome(const std::string &xuid, const std::string &username,
                              const std::string &name, const std::string &pos_json) {
        auto existing = queryRow("SELECT 1 FROM homes WHERE xuid = ? AND name = ?", {xuid, name});
        if (existing)
            return false;

        execute("INSERT INTO homes (xuid, username, name, pos) VALUES (?, ?, ?, ?)",
                {xuid, username, name, pos_json});
        return true;
    }

    bool ServerDB::deleteHome(const std::string &name, const std::string &username,
                              const std::string &xuid) {
        auto existing =
            queryRow("SELECT 1 FROM homes WHERE (xuid = ? OR username = ?) AND name = ?",
                     {xuid, username, name});
        if (!existing)
            return false;

        execute("DELETE FROM homes WHERE (xuid = ? OR username = ?) AND name = ?",
                {xuid, username, name});
        return true;
    }

    std::optional<Home> ServerDB::getHome(const std::string &name, const std::string &username,
                                          const std::string &xuid) {
        auto row = queryRow(
            "SELECT * FROM homes WHERE (xuid = ? OR username = ?) AND name = ?",
            {xuid, username, name});
        if (!row)
            return std::nullopt;

        Home h;
        h.xuid = (*row)["xuid"];
        h.username = (*row)["username"];
        h.name = (*row)["name"];
        h.pos = (*row)["pos"];
        h.cost = std::stod((*row)["cost"]);
        h.cooldown = std::stod((*row)["cooldown"]);
        h.delay = std::stod((*row)["delay"]);
        return h;
    }

    std::map<std::string, Home> ServerDB::getAllHomes(const std::string &username,
                                                      const std::string &xuid) {
        auto rows = query("SELECT * FROM homes WHERE xuid = ? OR username = ?", {xuid, username});
        std::map<std::string, Home> homes;
        for (auto &r : rows) {
            Home h;
            h.xuid = r["xuid"];
            h.username = r["username"];
            h.name = r["name"];
            h.pos = r["pos"];
            h.cost = std::stod(r["cost"]);
            h.cooldown = std::stod(r["cooldown"]);
            h.delay = std::stod(r["delay"]);
            homes[h.name] = h;
        }
        return homes;
    }

    HomeSettings ServerDB::getHomeSettings() {
        auto row = queryRow("SELECT * FROM home_settings WHERE id = 1");
        HomeSettings settings;
        if (row) {
            settings.delay = std::stod((*row)["delay"]);
            settings.cooldown = std::stod((*row)["cooldown"]);
            settings.cost = std::stod((*row)["cost"]);
        }
        return settings;
    }

    void ServerDB::setHomeSettings(double delay, double cooldown, double cost) {
        execute(
            "INSERT OR REPLACE INTO home_settings (id, delay, cooldown, cost) VALUES ('1', ?, ?, ?)",
            {std::to_string(delay), std::to_string(cooldown), std::to_string(cost)});
    }

    void ServerDB::setSpawn(const std::string &pos_json) {
        execute("INSERT OR REPLACE INTO spawns (id, pos) VALUES ('1', ?)", {pos_json});
    }

    std::optional<Spawn> ServerDB::getSpawn() {
        auto row = queryRow("SELECT * FROM spawns WHERE id = 1");
        if (!row)
            return std::nullopt;

        Spawn s;
        s.pos = (*row)["pos"];
        s.cost = std::stod((*row)["cost"]);
        s.cooldown = std::stod((*row)["cooldown"]);
        s.delay = std::stod((*row)["delay"]);
        return s;
    }

    std::string ServerDB::encodeLocation(double x, double y, double z,
                                         const std::string &dimension, float pitch, float yaw) {
        nlohmann::json j;
        j["x"] = x;
        j["y"] = y;
        j["z"] = z;
        j["dimension"] = dimension;
        j["pitch"] = pitch;
        j["yaw"] = yaw;
        return j.dump();
    }

    nlohmann::json ServerDB::decodeLocation(const std::string &pos_str) {
        return nlohmann::json::parse(pos_str);
    }

} // namespace primebds::db
