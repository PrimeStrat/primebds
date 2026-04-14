/// @file reloadscripts.cpp
/// Reloads server scripts!

#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"

namespace primebds::commands {

    static bool cmd_reloadscripts(PrimeBDS &, endstone::CommandSender &,
                        const std::vector<std::string> &);

    REGISTER_COMMAND(reloadscripts, "Reloads server scripts!", cmd_reloadscripts,
                     info.usages = {"/reloadscripts"};
                     info.permissions = {"primebds.command.reloadscripts"};
                     info.aliases = {"rscripts", "rs"};);

    /// Reloads server scripts!
    static bool cmd_reloadscripts(PrimeBDS &plugin, endstone::CommandSender &sender,
                                  const std::vector<std::string> &args) {
        plugin.getServer().reloadData();
        sender.sendMessage("\u00a7aServer scripts reloaded");
        return true;
    }

} // namespace primebds::commands
