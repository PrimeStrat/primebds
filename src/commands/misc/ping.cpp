#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"

namespace primebds::commands
{

    static bool cmd_ping(PrimeBDS &plugin, endstone::CommandSender &sender,
                         const std::vector<std::string> &args)
    {
        auto *player = dynamic_cast<endstone::Player *>(&sender);
        if (!player)
        {
            sender.sendMessage("This command can only be executed by a player");
            return false;
        }

        if (args.empty())
        {
            player->sendMessage("\u00a7ePing: \u00a7f" + std::to_string(player->getPing().count()) + "ms");
        }
        else
        {
            auto *target = plugin.getServer().getPlayer(args[0]);
            if (!target)
            {
                sender.sendMessage("\u00a7cPlayer not found");
                return false;
            }
            sender.sendMessage("\u00a7e" + target->getName() + "'s Ping: \u00a7f" +
                               std::to_string(target->getPing().count()) + "ms");
        }
        return true;
    }

    REGISTER_COMMAND(ping, "Check your or another player's latency!", cmd_ping,
                     info.usages = {"/ping [player: player]"};
                     info.permissions = {"primebds.command.ping"};);
} // namespace primebds::commands
