/// @file config_manager.h
/// Configuration management - loading, saving, and accessing config.json.

#pragma once

#include <nlohmann/json.hpp>
#include <mutex>
#include <string>
#include <optional>

namespace primebds::config {

    class ConfigManager {
    public:
        static ConfigManager &instance();

        void load();
        void save();
        void reload();

        nlohmann::json &config();
        const nlohmann::json &config() const;

        // Module accessors
        nlohmann::json getModule(const std::string &name) const;
        void setModule(const std::string &name, const nlohmann::json &value);

        // Commands config
        nlohmann::json loadCommandConfig();
        void saveCommandConfig(const nlohmann::json &config);
        bool isCommandEnabled(const std::string &name) const;

        // Permissions config
        nlohmann::json loadPermissions();
        void savePermissions(const nlohmann::json &perms);

        // Rules
        std::vector<std::string> loadRules();
        void saveRules(const std::vector<std::string> &rules);

        // Plain-text single-message files (no JSON escaping)
        std::string loadPlainText(const std::string &filename);
        void savePlainText(const std::string &filename, const std::string &content);

        // Server properties
        static std::string findServerProperties();
        static std::map<std::string, std::string> parsePropertiesFile(const std::string &path);

        // Path helpers
        static void setDataFolder(const std::string &path);
        static std::string getDataFolder();
        static std::string getConfigPath();

    private:
        ConfigManager();
        nlohmann::json config_;
        nlohmann::json command_config_;
        mutable std::mutex mutex_;
        std::string config_path_;
    };

} // namespace primebds::config
