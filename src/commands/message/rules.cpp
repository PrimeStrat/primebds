/// @file rules.cpp
/// Displays the server rules!

#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"
#include "primebds/utils/config/config_manager.h"

namespace primebds::commands {

    static bool cmd_rules(PrimeBDS &, endstone::CommandSender &,
                        const std::vector<std::string> &);

    REGISTER_COMMAND(rules, "Displays the server rules!", cmd_rules,
                     info.usages = {"/rules"};
                     info.permissions = {"primebds.command.rules"};);

    /// Displays the server rules!
    static bool cmd_rules(PrimeBDS &plugin, endstone::CommandSender &sender,
                          const std::vector<std::string> &args) {
        auto &cfg = config::ConfigManager::instance().config();
        auto rules_arr = cfg["modules"]["rules"]["list"];
        if (!rules_arr.is_array() || rules_arr.empty()) {
            sender.sendMessage("\u00a7eNo rules have been set");
            return true;
        }

        sender.sendMessage("\u00a76Server Rules:");
        int idx = 1;
        for (auto &r : rules_arr) {
            sender.sendMessage("\u00a7e" + std::to_string(idx++) + ". \u00a7f" + r.get<std::string>());
        }
        return true;
    }

} // namespace primebds::commands
