/// @file database_manager.h
/// Thread-safe SQLite database manager base class.

#pragma once

#include <mutex>
#include <string>
#include <vector>
#include <map>
#include <optional>
#include <functional>
#include <sqlite3.h>

namespace primebds::db {

    class DatabaseManager {
    public:
        explicit DatabaseManager(const std::string &db_name);
        virtual ~DatabaseManager();

        DatabaseManager(const DatabaseManager &) = delete;
        DatabaseManager &operator=(const DatabaseManager &) = delete;

        void execute(const std::string &query, const std::vector<std::string> &params = {});
        std::vector<std::map<std::string, std::string>> query(const std::string &sql,
                                                              const std::vector<std::string> &params = {});
        std::optional<std::map<std::string, std::string>> queryRow(const std::string &sql,
                                                                   const std::vector<std::string> &params = {});

        void createTable(const std::string &table,
                         const std::vector<std::pair<std::string, std::string>> &columns);

        void migrateTable(const std::string &table,
                          const std::vector<std::pair<std::string, std::string>> &expected_columns);

        std::vector<std::string> getColumnNames(const std::string &table);

        void close();

    protected:
        std::string db_path_;
        sqlite3 *db_ = nullptr;
        mutable std::mutex mutex_;

    private:
        void bindParams(sqlite3_stmt *stmt, const std::vector<std::string> &params);
    };

} // namespace primebds::db
