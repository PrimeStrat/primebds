#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"

namespace primebds::commands
{

    static bool cmd_cords(PrimeBDS &plugin, endstone::CommandSender &sender,
                          const std::vector<std::string> &args)
    {
        auto *player = dynamic_cast<endstone::Player *>(&sender);
        if (!player)
        {
            sender.sendMessage("\u00a7cOnly players can use this command.");
            return true;
        }
        auto loc = player->getLocation();
        player->sendMessage(std::to_string(loc.getBlockX()) + " " +
                            std::to_string(loc.getBlockY()) + " " +
                            std::to_string(loc.getBlockZ()));
        return true;
    }

    REGISTER_COMMAND(cords, "Print your current position!", cmd_cords,
                     info.usages = {"/cords"};
                     info.permissions = {"primebds.command.cords"};
                     info.aliases = {"blockpos", "pos"};);
} // namespace primebds::commands
