/// @file altspy.cpp
/// Toggle alt detection notifications!

#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"

namespace primebds::commands {

    static bool cmd_altspy(PrimeBDS &, endstone::CommandSender &,
                        const std::vector<std::string> &);

    REGISTER_COMMAND(altspy, "Toggle alt detection notifications!", cmd_altspy,
                     info.usages = {"/altspy"};
                     info.permissions = {"primebds.command.altspy"};);

    /// Toggle alt detection notifications!
    static bool cmd_altspy(PrimeBDS &plugin, endstone::CommandSender &sender,
                           const std::vector<std::string> &args) {
        auto *player = sender.asPlayer();
        if (!player) {
            sender.sendMessage("\u00a7cOnly players can use this command.");
            return true;
        }

        auto user = plugin.db->getOnlineUser(player->getXuid());
        bool enabled = user.has_value() && user->enabled_as;

        if (enabled) {
            plugin.db->updateUser(player->getXuid(), "enabled_as", "0");
            player->sendMessage("\u00a7cAltSpy disabled");
        } else {
            plugin.db->updateUser(player->getXuid(), "enabled_as", "1");
            player->sendMessage("\u00a7aAltSpy enabled");
        }
        return true;
    }

} // namespace primebds::commands
