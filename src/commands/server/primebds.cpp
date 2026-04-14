#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"
#include "primebds/utils/config/config_manager.h"

#include <sstream>

namespace primebds::commands
{

    namespace
    {
        // Resolve a dot-separated path like "modules.afk.enabled" in a JSON object.
        // Returns pointer to the value, or nullptr if any segment is missing.
        nlohmann::json *resolveJsonPath(nlohmann::json &root, const std::string &path)
        {
            nlohmann::json *node = &root;
            std::istringstream stream(path);
            std::string segment;
            while (std::getline(stream, segment, '.'))
            {
                if (!node->is_object() || !node->contains(segment))
                    return nullptr;
                node = &(*node)[segment];
            }
            return node;
        }

        const nlohmann::json *resolveJsonPath(const nlohmann::json &root, const std::string &path)
        {
            const nlohmann::json *node = &root;
            std::istringstream stream(path);
            std::string segment;
            while (std::getline(stream, segment, '.'))
            {
                if (!node->is_object() || !node->contains(segment))
                    return nullptr;
                node = &(*node).at(segment);
            }
            return node;
        }

        // Set a value at a dot-separated path, creating intermediate objects as needed.
        void setJsonPath(nlohmann::json &root, const std::string &path, const nlohmann::json &value)
        {
            nlohmann::json *node = &root;
            std::istringstream stream(path);
            std::string segment;
            std::string prev;
            std::vector<std::string> segments;
            while (std::getline(stream, segment, '.'))
                segments.push_back(segment);

            for (size_t i = 0; i < segments.size() - 1; ++i)
            {
                if (!node->contains(segments[i]) || !(*node)[segments[i]].is_object())
                    (*node)[segments[i]] = nlohmann::json::object();
                node = &(*node)[segments[i]];
            }
            (*node)[segments.back()] = value;
        }
    } // anonymous namespace

    static bool cmd_primebds(PrimeBDS &plugin, endstone::CommandSender &sender,
                             const std::vector<std::string> &args)
    {
        if (args.empty())
        {
            sender.sendMessage("\u00a7b--- PrimeBDS v3.4.0 ---");
            sender.sendMessage("\u00a77C++ Port of PrimeBDS for Endstone");
            sender.sendMessage("\u00a77Use /primebds <config|reloadconfig|info> for more options");
            return true;
        }

        std::string sub = args[0];
        for (auto &c : sub)
            c = (char)std::tolower(c);

        if (sub == "info")
        {
            sender.sendMessage("\u00a7b--- PrimeBDS Info ---");
            sender.sendMessage("\u00a77Version: \u00a7e3.4.0");
            sender.sendMessage("\u00a77Platform: \u00a7eC++ / Endstone");
            int online = (int)plugin.getServer().getOnlinePlayers().size();
            int max_p = plugin.getServer().getMaxPlayers();
            sender.sendMessage("\u00a77Players: \u00a7e" + std::to_string(online) + "/" + std::to_string(max_p));
            return true;
        }

        if (sub == "reloadconfig")
        {
            if (!sender.hasPermission("primebds.command.primebds.reloadconfig"))
            {
                sender.sendMessage("\u00a7cYou don't have permission to reload config");
                return false;
            }
            config::ConfigManager::instance().reload();
            sender.sendMessage("\u00a7aConfig reloaded!");
            return true;
        }

        if (sub == "config")
        {
            if (!sender.hasPermission("primebds.command.primebds.config"))
            {
                sender.sendMessage("\u00a7cYou don't have permission to modify config");
                return false;
            }

            if (args.size() < 2)
            {
                sender.sendMessage("\u00a7cUsage: /primebds config <key.path> [value]");
                sender.sendMessage("\u00a77Example: /primebds config modules.afk.enabled false");
                return false;
            }

            std::string key = args[1];

            if (args.size() == 2)
            {
                // Get value
                auto &cfg = config::ConfigManager::instance();
                auto &conf = cfg.config();
                auto *val = resolveJsonPath(conf, key);
                if (!val)
                {
                    sender.sendMessage("\u00a7cConfig key \u00a7e" + key + " \u00a7cnot found");
                }
                else
                {
                    sender.sendMessage("\u00a7e" + key + " \u00a7a= \u00a7e" + val->dump());
                }
                return true;
            }

            // Set value
            std::string value;
            for (size_t i = 2; i < args.size(); ++i)
            {
                if (i > 2)
                    value += " ";
                value += args[i];
            }

            // Try to parse as JSON, fallback to string
            nlohmann::json jval;
            try
            {
                jval = nlohmann::json::parse(value);
            }
            catch (...)
            {
                jval = value;
            }

            auto &cfg = config::ConfigManager::instance();
            setJsonPath(cfg.config(), key, jval);
            cfg.save();
            sender.sendMessage("\u00a7e" + key + " \u00a7aset to \u00a7e" + jval.dump());
            return true;
        }

        sender.sendMessage("\u00a7cUnknown subcommand. Use /primebds [config|reloadconfig|info]");
        return false;
    }

    REGISTER_COMMAND(primebds, "PrimeBDS management command!", cmd_primebds,
                     info.usages = {
                         "/primebds",
                         "/primebds (info)",
                         "/primebds (reloadconfig)",
                         "/primebds (config) <key.path: message> [value: message]"};
                     info.permissions = {"primebds.command.primebds"};);

} // namespace primebds::commands
