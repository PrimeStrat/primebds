#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"

#include <cmath>
#include <cstdio>
#include <string>
#include <variant>

namespace primebds::commands
{

    static void blockscan_tick(PrimeBDS &plugin, const std::string &player_name)
    {
        auto *player = plugin.getServer().getPlayer(player_name);
        if (!player)
        {
            auto it = plugin.blockscan_intervals.find(player_name);
            if (it != plugin.blockscan_intervals.end())
            {
                plugin.getServer().getScheduler().cancelTask(it->second);
                plugin.blockscan_intervals.erase(it);
            }
            return;
        }

        auto &dim = player->getDimension();
        auto loc = player->getLocation();

        double yaw = loc.getYaw() * 3.14159265 / 180.0;
        double pitch = loc.getPitch() * 3.14159265 / 180.0;
        double dir_x = -std::sin(yaw) * std::cos(pitch);
        double dir_y = -std::sin(pitch);
        double dir_z = std::cos(yaw) * std::cos(pitch);

        double eye_x = loc.getX();
        double eye_y = loc.getY() + 1.0;
        double eye_z = loc.getZ();

        constexpr double max_distance = 6.0;
        constexpr double step = 0.2;

        for (double d = 0.0; d <= max_distance; d += step)
        {
            int bx = static_cast<int>(std::floor(eye_x + dir_x * d));
            int by = static_cast<int>(std::floor(eye_y + dir_y * d));
            int bz = static_cast<int>(std::floor(eye_z + dir_z * d));

            auto block = dim.getBlockAt(bx, by, bz);
            if (block && block->getType() != "minecraft:air")
            {
                auto bl = block->getLocation();
                auto data = block->getData();
                std::string block_type = block->getType();

                // Build states text
                std::string states_text;
                if (data)
                {
                    auto states = data->getBlockStates();
                    if (states.empty())
                    {
                        states_text = "  \u00a77- (none)";
                    }
                    else
                    {
                        for (auto &[key, val] : states)
                        {
                            states_text += "  \u00a77- \u00a7f" + key + " = ";
                            std::visit([&states_text](auto &&v)
                                       {
                                using T = std::decay_t<decltype(v)>;
                                if constexpr (std::is_same_v<T, bool>)
                                    states_text += v ? "true" : "false";
                                else if constexpr (std::is_same_v<T, int>)
                                    states_text += std::to_string(v);
                                else if constexpr (std::is_same_v<T, std::string>)
                                    states_text += v; }, val);
                            states_text += "\n";
                        }
                    }

                    char buf[1024];
                    std::snprintf(buf, sizeof(buf),
                                  "\u00a7bBlock Information\u00a7r\n"
                                  "\u00a77- \u00a7eX: \u00a7f%d\n"
                                  "\u00a77- \u00a7eY: \u00a7f%d\n"
                                  "\u00a77- \u00a7eZ: \u00a7f%d\n"
                                  "\u00a77- \u00a7eType: \u00a7f%s\n"
                                  "\u00a77- \u00a7eStates:\n%s"
                                  "\u00a77- \u00a7eRuntime ID: \u00a7f%u",
                                  bl.getBlockX(), bl.getBlockY(), bl.getBlockZ(),
                                  block_type.c_str(),
                                  states_text.c_str(),
                                  data->getRuntimeId());
                    player->sendTip(buf);
                }
                return;
            }
        }

        player->sendTip("\u00a7cNo block in sight");
    }

    static bool cmd_blockscan(PrimeBDS &plugin, endstone::CommandSender &sender,
                              const std::vector<std::string> &args)
    {
        auto *player = dynamic_cast<endstone::Player *>(&sender);
        if (!player)
        {
            sender.sendMessage("\u00a7cOnly players can use this command.");
            return true;
        }

        std::string name = player->getName();

        // Toggle off if already active
        auto it = plugin.blockscan_intervals.find(name);
        if (it != plugin.blockscan_intervals.end())
        {
            plugin.getServer().getScheduler().cancelTask(it->second);
            plugin.blockscan_intervals.erase(it);
            sender.sendMessage("\u00a7cBlock scanning disabled");
            return true;
        }

        // Check for explicit disable arg
        if (!args.empty() && args[0] == "disable")
        {
            sender.sendMessage("\u00a7cBlock scanning is not active");
            return true;
        }

        // Schedule repeating task every 10 ticks (0.5s)
        auto task = plugin.getServer().getScheduler().runTaskTimer(
            plugin,
            [&plugin, name]()
            {
                blockscan_tick(plugin, name);
            },
            0, 10);

        if (task)
        {
            plugin.blockscan_intervals[name] = task->getTaskId();
            sender.sendMessage("\u00a7aBlock scanning enabled");
        }
        else
        {
            sender.sendMessage("\u00a7cFailed to start block scanning task");
        }
        return true;
    }

    REGISTER_COMMAND(blockscan, "Continuously show information about the block you're looking at.", cmd_blockscan,
                     info.usages = {"/blockscan (disable)"};
                     info.permissions = {"primebds.command.blockscan"};);
} // namespace primebds::commands
