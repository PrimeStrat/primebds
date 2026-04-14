/// @file database_manager.cpp
/// Thread-safe SQLite database manager implementation.

#include "primebds/utils/database/database_manager.h"

#include <filesystem>
#include <set>
#include <stdexcept>
#include <iostream>

namespace primebds::db {

    DatabaseManager::DatabaseManager(const std::string &db_path) {
        namespace fs = std::filesystem;
        db_path_ = db_path;
        if (db_path_.find(".db") == std::string::npos) {
            db_path_ += ".db";
        }

        // Ensure parent directory exists
        fs::create_directories(fs::path(db_path_).parent_path());

        int rc = sqlite3_open(db_path_.c_str(), &db_);
        if (rc != SQLITE_OK) {
            throw std::runtime_error("Failed to open database: " + std::string(sqlite3_errmsg(db_)));
        }

        // Enable WAL mode for concurrency
        execute("PRAGMA journal_mode=WAL;");
    }

    DatabaseManager::~DatabaseManager() {
        close();
    }

    void DatabaseManager::close() {
        std::lock_guard lock(mutex_);
        if (db_) {
            sqlite3_close(db_);
            db_ = nullptr;
        }
    }

    void DatabaseManager::bindParams(sqlite3_stmt *stmt, const std::vector<std::string> &params) {
        for (int i = 0; i < static_cast<int>(params.size()); ++i) {
            sqlite3_bind_text(stmt, i + 1, params[i].c_str(), -1, SQLITE_TRANSIENT);
        }
    }

    void DatabaseManager::execute(const std::string &sql, const std::vector<std::string> &params) {
        std::lock_guard lock(mutex_);

        sqlite3_stmt *stmt = nullptr;
        int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
        if (rc != SQLITE_OK) {
            throw std::runtime_error("SQL prepare error: " + std::string(sqlite3_errmsg(db_)) +
                                     " | Query: " + sql);
        }

        bindParams(stmt, params);

        rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        if (rc != SQLITE_DONE && rc != SQLITE_ROW) {
            throw std::runtime_error("SQL execute error: " + std::string(sqlite3_errmsg(db_)));
        }
    }

    std::vector<std::map<std::string, std::string>>
    DatabaseManager::query(const std::string &sql, const std::vector<std::string> &params) {
        std::lock_guard lock(mutex_);

        sqlite3_stmt *stmt = nullptr;
        int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
        if (rc != SQLITE_OK) {
            throw std::runtime_error("SQL prepare error: " + std::string(sqlite3_errmsg(db_)));
        }

        bindParams(stmt, params);

        std::vector<std::map<std::string, std::string>> results;
        int col_count = sqlite3_column_count(stmt);

        while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
            std::map<std::string, std::string> row;
            for (int i = 0; i < col_count; ++i) {
                auto col_name = sqlite3_column_name(stmt, i);
                auto col_val = reinterpret_cast<const char *>(sqlite3_column_text(stmt, i));
                row[col_name] = col_val ? col_val : "";
            }
            results.push_back(std::move(row));
        }

        sqlite3_finalize(stmt);
        return results;
    }

    std::optional<std::map<std::string, std::string>>
    DatabaseManager::queryRow(const std::string &sql, const std::vector<std::string> &params) {
        auto results = query(sql, params);
        if (results.empty())
            return std::nullopt;
        return results[0];
    }

    void DatabaseManager::createTable(const std::string &table,
                                      const std::vector<std::pair<std::string, std::string>> &columns) {
        std::string sql = "CREATE TABLE IF NOT EXISTS " + table + " (";
        for (size_t i = 0; i < columns.size(); ++i) {
            if (i > 0)
                sql += ", ";
            sql += columns[i].first + " " + columns[i].second;
        }
        sql += ")";
        execute(sql);
    }

    void DatabaseManager::migrateTable(const std::string &table,
                                       const std::vector<std::pair<std::string, std::string>> &expected_columns) {
        auto existing = getColumnNames(table);
        std::set<std::string> existing_set(existing.begin(), existing.end());

        for (auto &[col_name, col_type] : expected_columns) {
            if (existing_set.find(col_name) == existing_set.end()) {
                std::string default_val;
                if (col_type.find("INTEGER") != std::string::npos) {
                    default_val = col_name.substr(0, 4) == "can_" ? "1" : "0";
                } else if (col_type.find("REAL") != std::string::npos) {
                    default_val = "0.0";
                } else {
                    default_val = "''";
                }

                try {
                    execute("ALTER TABLE " + table + " ADD COLUMN " + col_name + " " + col_type +
                            " DEFAULT " + default_val);
                } catch (const std::exception &e) {
                    std::cerr << "Warning: Could not add column '" << col_name << "' to " << table
                              << ": " << e.what() << std::endl;
                }
            }
        }
    }

    std::vector<std::string> DatabaseManager::getColumnNames(const std::string &table) {
        std::vector<std::string> names;
        auto rows = query("PRAGMA table_info(" + table + ")");
        for (auto &row : rows) {
            auto it = row.find("name");
            if (it != row.end()) {
                names.push_back(it->second);
            }
        }
        return names;
    }

} // namespace primebds::db
