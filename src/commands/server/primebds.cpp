#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"
#include "primebds/utils/config/config_manager.h"

namespace primebds::commands
{

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
                sender.sendMessage("\u00a7cUsage: /primebds config <key> [value]");
                return false;
            }

            std::string key = args[1];

            if (args.size() == 2)
            {
                // Get value
                auto &cfg = config::ConfigManager::instance();
                auto conf = cfg.config();
                if (!conf.contains(key))
                {
                    sender.sendMessage("\u00a7cConfig key \u00a7e" + key + " \u00a7cnot found");
                }
                else
                {
                    sender.sendMessage("\u00a7e" + key + " \u00a7a= \u00a7e" + conf[key].dump());
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
            cfg.config()[key] = jval;
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
                         "/primebds (config) <key: message> [value: message]"};
                     info.permissions = {"primebds.command.primebds"};);

} // namespace primebds::commands
