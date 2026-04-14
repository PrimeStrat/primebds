/// @file globalmute.cpp
/// Toggles global mute for the server!

#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"

namespace primebds::commands {

    static bool cmd_globalmute(PrimeBDS &, endstone::CommandSender &,
                        const std::vector<std::string> &);

    REGISTER_COMMAND(globalmute, "Toggles global mute for the server!", cmd_globalmute,
                     info.usages = {"/globalmute"};
                     info.permissions = {"primebds.command.globalmute"};
                     info.aliases = {"gmute"};);

    /// Toggles global mute for the server!
    static bool cmd_globalmute(PrimeBDS &plugin, endstone::CommandSender &sender,
                               const std::vector<std::string> &args) {
        plugin.globalmute = plugin.globalmute ? 0 : 1;
        if (plugin.globalmute) {
            plugin.getServer().broadcastMessage("\u00a7c\u00a7lGlobal mute has been enabled by " + sender.getName());
        } else {
            plugin.getServer().broadcastMessage("\u00a7a\u00a7lGlobal mute has been disabled by " + sender.getName());
        }
        return true;
    }

} // namespace primebds::commands
