/// @file voice.cpp
/// Enables voice chat attachment!

#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"

namespace primebds::commands {

    static bool cmd_voice(PrimeBDS &, endstone::CommandSender &,
                        const std::vector<std::string> &);

    REGISTER_COMMAND(voice, "Enables voice chat attachment!", cmd_voice,
                     info.usages = {"/voice"};
                     info.permissions = {"primebds.command.voice"};);

    /// Enables voice chat attachment!
    static bool cmd_voice(PrimeBDS &plugin, endstone::CommandSender &sender,
                          const std::vector<std::string> &args) {
        auto *player = sender.asPlayer();
        if (!player) {
            sender.sendMessage("\u00a7cOnly players can use this command.");
            return true;
        }

        // Check if global mute is active and player doesn't have exempt
        if (plugin.globalmute && !player->hasPermission("primebds.globalmute.exempt")) {
            sender.sendMessage("\u00a7cGlobal mute is active. You cannot use voice chat.");
            return true;
        }

        sender.sendMessage("\u00a7aVoice attachment enabled");
        return true;
    }

} // namespace primebds::commands
