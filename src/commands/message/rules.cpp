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
        auto rules = config::ConfigManager::instance().loadRules();
        for (auto &r : rules) {
            sender.sendMessage(r);
        }
        return true;
    }

} // namespace primebds::commands
