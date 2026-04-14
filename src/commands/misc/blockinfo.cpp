#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"

#include <cmath>
#include <string>

namespace primebds::commands
{

    static bool cmd_blockinfo(PrimeBDS &plugin, endstone::CommandSender &sender,
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

        double yaw = loc.getYaw() * 3.14159265 / 180.0;
        double pitch = loc.getPitch() * 3.14159265 / 180.0;
        double dir_x = -std::sin(yaw) * std::cos(pitch);
        double dir_y = -std::sin(pitch);
        double dir_z = std::cos(yaw) * std::cos(pitch);

        for (double d = 0.0; d <= 6.0; d += 0.2)
        {
            int bx = static_cast<int>(loc.getX() + dir_x * d);
            int by = static_cast<int>(loc.getY() + dir_y * d);
            int bz = static_cast<int>(loc.getZ() + dir_z * d);
            auto block = dim.getBlockAt(bx, by, bz);
            if (block->getType() != "minecraft:air")
            {
                auto bl = block->getLocation();
                std::string msg = "\u00a7bBlock Information:\n";
                msg += "\u00a77- \u00a7eX: \u00a7f" + std::to_string(bl.getBlockX()) + "\n";
                msg += "\u00a77- \u00a7eY: \u00a7f" + std::to_string(bl.getBlockY()) + "\n";
                msg += "\u00a77- \u00a7eZ: \u00a7f" + std::to_string(bl.getBlockZ()) + "\n";
                msg += "\u00a77- \u00a7eType: \u00a7f" + block->getType() + "\n";
                player->sendMessage(msg);
                return true;
            }
        }
        player->sendMessage("\u00a7cNo block in sight");
        return true;
    }

    REGISTER_COMMAND(blockinfo, "Prints info of the facing block!", cmd_blockinfo,
                     info.usages = {"/blockinfo"};
                     info.permissions = {"primebds.command.blockinfo"};);
} // namespace primebds::commands
