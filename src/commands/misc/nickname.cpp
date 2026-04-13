#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"

namespace primebds::commands
{

    static bool cmd_nickname(PrimeBDS &plugin, endstone::CommandSender &sender,
                             const std::vector<std::string> &args)
    {
        auto *player = dynamic_cast<endstone::Player *>(&sender);
        if (!player)
        {
            sender.sendMessage("\u00a7cOnly players can use this command.");
            return false;
        }

        if (args.empty())
        {
            player->setNameTag(player->getName());
            player->sendMessage("\u00a7eNickname cleared");
            return true;
        }
        std::string nick = args[0];
        player->setNameTag(nick);
        player->sendMessage("\u00a7eNickname set to: " + nick);
        return true;
    }

    REGISTER_COMMAND(nickname, "Set your display name!", cmd_nickname,
                     info.usages = {"/nickname [name: string]"};
                     info.permissions = {"primebds.command.nickname"};
                     info.aliases = {"nick"};);
} // namespace primebds::commands
