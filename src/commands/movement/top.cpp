#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"

namespace primebds::commands
{

    static bool cmd_top(PrimeBDS &plugin, endstone::CommandSender &sender,
                        const std::vector<std::string> &args)
    {
        auto *player = sender.asPlayer();
        if (!player)
        {
            sender.sendMessage("\u00a7cOnly players can use this command.");
            return true;
        }

        auto &dim = player->getDimension();
        auto loc = player->getLocation();
        int x = (int)loc.getX(), z = (int)loc.getZ();

        std::string dim_name = dim.getName();
        int world_height = (dim_name == "Nether") ? 120 : (dim_name == "TheEnd") ? 255
                                                                                 : 320;

        int highest_y = std::min(dim.getHighestBlockYAt(x, z), world_height);
        if (highest_y < -63)
        {
            sender.sendMessage("\u00a7cNo valid open-air block found at this X, Z position.");
            return false;
        }

        // Check for valid teleport spot with 2 air blocks above
        for (int y = highest_y; y >= highest_y - 5 && y >= -63; --y)
        {
            if (dim.getBlockAt(x, y + 1, z)->getType() == "minecraft:air" &&
                dim.getBlockAt(x, y + 2, z)->getType() == "minecraft:air")
            {
                player->performCommand("tp " + std::to_string(x) + " " + std::to_string(y + 1) + " " + std::to_string(z));
                return true;
            }
        }

        sender.sendMessage("\u00a7cNo valid open-air block found at this X, Z position.");
        return false;
    }

    REGISTER_COMMAND(top, "Warps you to the topmost block with air!", cmd_top,
                     info.usages = {"/top"};
                     info.permissions = {"primebds.command.top"};);
} // namespace primebds::commands
