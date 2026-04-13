/// @file config_manager.cpp
/// Configuration management implementation.

#include "primebds/utils/config/config_manager.h"
#include "primebds/utils/config/config_defaults.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

namespace fs = std::filesystem;

namespace primebds::config
{

    namespace
    {

        std::string findDataFolder()
        {
            // Walk up from cwd to find plugins/ and worlds/ dirs
            auto current = fs::current_path();
            while (true)
            {
                if (fs::exists(current / "plugins") && fs::exists(current / "worlds"))
                {
                    auto data = current / "plugins" / "primebds_data";
                    fs::create_directories(data);
                    return data.string();
                }
                auto parent = current.parent_path();
                if (parent == current)
                    break;
                current = parent;
            }
            auto fallback = fs::current_path() / "plugins" / "primebds_data";
            fs::create_directories(fallback);
            return fallback.string();
        }

        std::string readTextFile(const std::string &path)
        {
            std::ifstream f(path);
            if (!f.is_open())
                return {};
            std::stringstream ss;
            ss << f.rdbuf();
            return ss.str();
        }

        bool writeTextFile(const std::string &path, const std::string &content)
        {
            fs::create_directories(fs::path(path).parent_path());
            std::ofstream f(path);
            if (!f.is_open())
                return false;
            f << content;
            return f.good();
        }

    } // anonymous namespace

    ConfigManager &ConfigManager::instance()
    {
        static ConfigManager inst;
        return inst;
    }

    ConfigManager::ConfigManager()
    {
        config_path_ = (fs::path(findDataFolder()) / "config.json").string();
        load();
    }

    std::string ConfigManager::getDataFolder()
    {
        return findDataFolder();
    }

    std::string ConfigManager::getConfigPath()
    {
        return (fs::path(findDataFolder()) / "config.json").string();
    }

    void ConfigManager::load()
    {
        std::lock_guard lock(mutex_);

        auto content = readTextFile(config_path_);
        if (content.empty())
        {
            config_ = {{"commands", nlohmann::json::object()}, {"modules", getDefaultModules()}};
            std::cout << "[PrimeBDS] Config file not found, using defaults.\n";
            return;
        }

        try
        {
            config_ = nlohmann::json::parse(content);
        }
        catch (const nlohmann::json::exception &e)
        {
            std::cout << "[PrimeBDS] JSON error in config.json: " << e.what() << ". Using defaults.\n";
            config_ = {{"commands", nlohmann::json::object()}, {"modules", getDefaultModules()}};
        }

        // Ensure modules section exists with defaults for missing keys
        if (!config_.contains("modules"))
            config_["modules"] = getDefaultModules();
        else
        {
            auto defaults = getDefaultModules();
            for (auto &[key, val] : defaults.items())
            {
                if (!config_["modules"].contains(key))
                    config_["modules"][key] = val;
            }
        }
    }

    void ConfigManager::save()
    {
        std::lock_guard lock(mutex_);
        writeTextFile(config_path_, config_.dump(4));
    }

    void ConfigManager::reload()
    {
        {
            std::lock_guard lock(mutex_);
            config_ = {};
        }
        load();
    }

    nlohmann::json &ConfigManager::config()
    {
        return config_;
    }

    const nlohmann::json &ConfigManager::config() const
    {
        return config_;
    }

    nlohmann::json ConfigManager::getModule(const std::string &name) const
    {
        std::lock_guard lock(mutex_);
        if (config_.contains("modules") && config_["modules"].contains(name))
            return config_["modules"][name];
        auto defaults = getDefaultModules();
        if (defaults.contains(name))
            return defaults[name];
        return {};
    }

    void ConfigManager::setModule(const std::string &name, const nlohmann::json &value)
    {
        std::lock_guard lock(mutex_);
        config_["modules"][name] = value;
    }

    nlohmann::json ConfigManager::loadCommandConfig()
    {
        auto path = (fs::path(findDataFolder()) / "commands.json").string();
        auto content = readTextFile(path);
        if (content.empty())
            return nlohmann::json::object();

        try
        {
            return nlohmann::json::parse(content);
        }
        catch (...)
        {
            return nlohmann::json::object();
        }
    }

    void ConfigManager::saveCommandConfig(const nlohmann::json &config)
    {
        auto path = (fs::path(findDataFolder()) / "commands.json").string();
        writeTextFile(path, config.dump(4));
    }

    nlohmann::json ConfigManager::loadPermissions()
    {
        auto path = (fs::path(findDataFolder()) / "permissions.json").string();
        auto content = readTextFile(path);
        if (content.empty())
        {
            auto defaults = getDefaultPermissions();
            writeTextFile(path, defaults.dump(4));
            return defaults;
        }

        try
        {
            auto perms = nlohmann::json::parse(content);
            // Ensure Default and Operator exist
            bool updated = false;
            auto defaults = getDefaultPermissions();
            if (!perms.contains("Default"))
            {
                perms["Default"] = defaults["Default"];
                updated = true;
            }
            if (!perms.contains("Operator"))
            {
                perms["Operator"] = defaults["Operator"];
                updated = true;
            }
            if (updated)
                writeTextFile(path, perms.dump(4));
            return perms;
        }
        catch (...)
        {
            auto defaults = getDefaultPermissions();
            writeTextFile(path, defaults.dump(4));
            return defaults;
        }
    }

    void ConfigManager::savePermissions(const nlohmann::json &perms)
    {
        auto path = (fs::path(findDataFolder()) / "permissions.json").string();
        writeTextFile(path, perms.dump(4));
    }

    std::vector<std::string> ConfigManager::loadRules()
    {
        auto path = (fs::path(findDataFolder()) / "rules.txt").string();
        auto content = readTextFile(path);
        if (content.empty())
            return {};

        std::vector<std::string> rules;
        std::istringstream stream(content);
        std::string line;
        while (std::getline(stream, line))
        {
            // Trim whitespace
            auto start = line.find_first_not_of(" \t\r\n");
            if (start == std::string::npos)
                continue;
            auto end = line.find_last_not_of(" \t\r\n");
            rules.push_back(line.substr(start, end - start + 1));
        }
        return rules;
    }

    void ConfigManager::saveRules(const std::vector<std::string> &rules)
    {
        auto path = (fs::path(findDataFolder()) / "rules.txt").string();
        std::string content;
        for (auto &r : rules)
            content += r + "\n";
        writeTextFile(path, content);
    }

    std::string ConfigManager::findServerProperties()
    {
        auto current = fs::current_path();
        while (true)
        {
            auto candidate = current / "server.properties";
            if (fs::exists(candidate))
                return candidate.string();
            auto parent = current.parent_path();
            if (parent == current)
                break;
            current = parent;
        }
        return {};
    }

    std::map<std::string, std::string> ConfigManager::parsePropertiesFile(const std::string &path)
    {
        std::map<std::string, std::string> props;
        auto content = readTextFile(path);
        if (content.empty())
            return props;

        std::istringstream stream(content);
        std::string line;
        while (std::getline(stream, line))
        {
            auto trimmed = line;
            auto s = trimmed.find_first_not_of(" \t\r\n");
            if (s == std::string::npos || trimmed[s] == '#')
                continue;
            auto eq = trimmed.find('=');
            if (eq == std::string::npos)
                continue;
            auto key = trimmed.substr(0, eq);
            auto val = trimmed.substr(eq + 1);
            // Trim
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            val.erase(0, val.find_first_not_of(" \t"));
            val.erase(val.find_last_not_of(" \t\r\n") + 1);
            props[key] = val;
        }
        return props;
    }

} // namespace primebds::config
