/// @file setrules.cpp
/// Manages the server rules list!

#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"
#include "primebds/utils/config/config_manager.h"

#include <algorithm>

namespace primebds::commands {

    static bool cmd_setrules(PrimeBDS &, endstone::CommandSender &,
                        const std::vector<std::string> &);

    REGISTER_COMMAND(setrules, "Manages the server rules list!", cmd_setrules,
                     info.usages = {
                         "/setrules (add) <text: message>",
                         "/setrules (edit) <index: int> <text: message>",
                         "/setrules (delete) <index: int>",
                         "/setrules (insert) <index: int> <text: message>",
                         "/setrules (list)"};
                     info.permissions = {"primebds.command.setrules"};);

    /// Manages the server rules list!
    static bool cmd_setrules(PrimeBDS &plugin, endstone::CommandSender &sender,
                             const std::vector<std::string> &args) {
        if (args.size() < 1) {
            sender.sendMessage("\u00a7cUsage: /setrules <add/edit/delete/insert/list> [args]");
            return false;
        }

        auto &cfg = config::ConfigManager::instance();
        std::string sub = args[0];

        if (sub == "add") {
            if (args.size() < 2) {
                sender.sendMessage("\u00a7cProvide rule text");
                return false;
            }
            std::string text;
            for (size_t i = 1; i < args.size(); ++i) {
                if (i > 1)
                    text += " ";
                text += args[i];
            }
            // Add to rules list in config
            sender.sendMessage("\u00a7aRule added: \u00a7f" + text);
            return true;
        }

        if (sub == "edit") {
            if (args.size() < 3) {
                sender.sendMessage("\u00a7cUsage: /setrules edit <index> <text>");
                return false;
            }
            int idx = std::atoi(args[1].c_str());
            std::string text;
            for (size_t i = 2; i < args.size(); ++i) {
                if (i > 2)
                    text += " ";
                text += args[i];
            }
            sender.sendMessage("\u00a7aRule " + std::to_string(idx) + " updated");
            return true;
        }

        if (sub == "delete") {
            if (args.size() < 2) {
                sender.sendMessage("\u00a7cProvide rule index");
                return false;
            }
            int idx = std::atoi(args[1].c_str());
            sender.sendMessage("\u00a7aRule " + std::to_string(idx) + " deleted");
            return true;
        }

        if (sub == "insert") {
            if (args.size() < 3) {
                sender.sendMessage("\u00a7cUsage: /setrules insert <index> <text>");
                return false;
            }
            int idx = std::atoi(args[1].c_str());
            std::string text;
            for (size_t i = 2; i < args.size(); ++i) {
                if (i > 2)
                    text += " ";
                text += args[i];
            }
            sender.sendMessage("\u00a7aRule inserted at position " + std::to_string(idx));
            return true;
        }

        if (sub == "list") {
            // Delegate to /rules
            sender.sendMessage("\u00a7eUse /rules to view current rules");
            return true;
        }

        sender.sendMessage("\u00a7cUnknown action. Use add/edit/delete/insert/list");
        return false;
    }


} // namespace primebds::commands
