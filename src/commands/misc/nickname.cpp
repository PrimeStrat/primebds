/// @file nickname.cpp
/// Set your display name!

#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"

namespace primebds::commands {

    static bool cmd_nickname(PrimeBDS &, endstone::CommandSender &,
                        const std::vector<std::string> &);

    REGISTER_COMMAND(nickname, "Set your display name!", cmd_nickname,
                     info.usages = {"/nickname [name: string]"};
                     info.permissions = {"primebds.command.nickname"};
                     info.aliases = {"nick"};);

    /// Set your display name!
    static bool cmd_nickname(PrimeBDS &plugin, endstone::CommandSender &sender,
                             const std::vector<std::string> &args) {
        auto *player = sender.asPlayer();
        if (!player) {
            sender.sendMessage("\u00a7cOnly players can use this command.");
            return false;
        }

        if (args.empty()) {
            player->setNameTag(player->getName());
            player->sendMessage("\u00a7eNickname cleared");
            return true;
        }
        std::string nick = args[0];
        player->setNameTag(nick);
        player->sendMessage("\u00a7eNickname set to: " + nick);
        return true;
    }

} // namespace primebds::commands
