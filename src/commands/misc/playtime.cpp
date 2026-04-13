#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"

namespace primebds::commands
{

    static bool cmd_playtime(PrimeBDS &plugin, endstone::CommandSender &sender,
                             const std::vector<std::string> &args)
    {
        auto *player = dynamic_cast<endstone::Player *>(&sender);
        if (!player)
        {
            sender.sendMessage("This command can only be executed by a player");
            return false;
        }

        std::string xuid;
        std::string name;
        if (args.empty())
        {
            xuid = player->getXuid();
            name = player->getName();
        }
        else
        {
            auto user = plugin.db->getUserByName(args[0]);
            if (!user.has_value())
            {
                sender.sendMessage("\u00a7cPlayer not found");
                return false;
            }
            xuid = user->xuid;
            name = user->name;
        }

        int64_t total = plugin.sldb->getTotalPlaytime(xuid);
        int64_t hours = total / 3600;
        int64_t minutes = (total % 3600) / 60;
        sender.sendMessage("\u00a7e" + name + "'s \u00a7rTotal Playtime: \u00a7f" +
                           std::to_string(hours) + "h " + std::to_string(minutes) + "m");
        return true;
    }

    REGISTER_COMMAND(playtime, "Check total playtime!", cmd_playtime,
                     info.usages = {"/playtime [player: player]"};
                     info.permissions = {"primebds.command.playtime"};);
} // namespace primebds::commands
