/// @file bottom.cpp
/// Teleport to the lowest safe position!

#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"

namespace primebds::commands {

    static bool cmd_bottom(PrimeBDS &, endstone::CommandSender &,
                        const std::vector<std::string> &);

    REGISTER_COMMAND(bottom, "Teleport to the lowest safe position!", cmd_bottom,
                     info.usages = {"/bottom"};
                     info.permissions = {"primebds.command.bottom"};);

    /// Teleport to the lowest safe position!
    static bool cmd_bottom(PrimeBDS &plugin, endstone::CommandSender &sender,
                           const std::vector<std::string> &args) {
        auto *player = sender.asPlayer();
        if (!player) {
            sender.sendMessage("\u00a7cOnly players can use this command.");
            return true;
        }

        auto &dim = player->getDimension();
        auto loc = player->getLocation();
        int x = (int)loc.getX(), z = (int)loc.getZ();

        // Determine dimension bounds
        int min_y = -63;
        std::string dim_name = dim.getName();
        if (dim_name == "Nether")
            min_y = 0;
        else if (dim_name == "TheEnd")
            min_y = 0;

        int current_y = (int)loc.getY();

        // Scan downward for air pocket above solid
        for (int y = min_y; y < current_y; ++y) {
            auto block = dim.getBlockAt(x, y, z);
            auto above1 = dim.getBlockAt(x, y + 1, z);
            auto above2 = dim.getBlockAt(x, y + 2, z);
            if (block->getType() != "minecraft:air" &&
                above1->getType() == "minecraft:air" &&
                above2->getType() == "minecraft:air") {
                player->performCommand("tp " + std::to_string(x) + " " + std::to_string(y + 1) + " " + std::to_string(z));
                player->sendMessage("\u00a7aTeleported to the bottom");
                return true;
            }
        }

        sender.sendMessage("\u00a7cNo valid position found below you");
        return false;
    }

} // namespace primebds::commands
