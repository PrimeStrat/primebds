#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"

#include <cstdio>

namespace primebds::commands
{

    static const char *get_tps_color(float tps)
    {
        if (tps > 18.0f)
            return "\u00a7a"; // green
        if (tps >= 14.0f)
            return "\u00a7e"; // yellow
        return "\u00a7c";     // red
    }

    static const char *get_ping_color(int ping)
    {
        if (ping <= 80)
            return "\u00a7a"; // green
        if (ping <= 160)
            return "\u00a7e"; // yellow
        return "\u00a7c";     // red
    }

    static const char *get_mspt_color(float mspt)
    {
        return mspt < 50.0f ? "\u00a7a" : "\u00a7c";
    }

    static const char *get_entity_color(int count)
    {
        if (count < 600)
            return "\u00a7a"; // green
        if (count <= 800)
            return "\u00a7e"; // yellow
        return "\u00a7c";     // red
    }

    static const char *get_dim_color(const std::string &dim_name)
    {
        if (dim_name == "Overworld")
            return "\u00a7a";
        if (dim_name == "Nether")
            return "\u00a7c";
        if (dim_name == "TheEnd")
            return "\u00a77";
        return "\u00a78";
    }

    static void monitor_tick(PrimeBDS &plugin, const std::string &player_name)
    {
        auto *player = plugin.getServer().getPlayer(player_name);
        if (!player)
        {
            plugin.monitor_intervals.erase(player_name);
            return;
        }

        auto &server = plugin.getServer();
        auto *level = server.getLevel();
        if (!level)
            return;

        float tps = server.getAverageTicksPerSecond();
        float mspt_avg = server.getAverageMillisecondsPerTick();
        float mspt_cur = server.getCurrentMillisecondsPerTick();
        float tick_usage = server.getAverageTickUsage();
        int entity_count = static_cast<int>(level->getActors().size());
        std::string server_version = server.getMinecraftVersion();
        std::string level_name = level->getName();

        // Chunk counts per dimension
        auto *overworld = level->getDimension("Overworld");
        auto *nether = level->getDimension("Nether");
        auto *the_end = level->getDimension("TheEnd");
        int overworld_chunks = overworld ? static_cast<int>(overworld->getLoadedChunks().size()) : 0;
        int nether_chunks = nether ? static_cast<int>(nether->getLoadedChunks().size()) : 0;
        int end_chunks = the_end ? static_cast<int>(the_end->getLoadedChunks().size()) : 0;

        // Player stats
        int ping = static_cast<int>(player->getPing().count());
        std::string player_dim = player->getDimension().getName();

        int tps_int = static_cast<int>(tps);
        int tps_frac = static_cast<int>((tps - tps_int) * 10);

        char buf[1024];
        std::snprintf(buf, sizeof(buf),
                      "\u00a7bServer Monitor\u00a7r\n"
                      "\u00a7r---------------------------\n"
                      "\u00a7rLevel: %s \u00a7o\u00a77(ver. \u00a7a%s\u00a77)\n"
                      "\u00a7rTPS: %s%d.%d \u00a7o\u00a77(%.1f%%)\u00a7r\n"
                      "\u00a7rMSPT: %s%.1fms \u00a7o\u00a77(avg) \u00a7r\u00a77| %s%.1fms \u00a7o\u00a77(cur)\n"
                      "\u00a7rLoaded Chunks: \u00a7a%d \u00a77| \u00a7c%d \u00a77| \u00a77%d\n"
                      "\u00a7rLoaded Entities: %s%d\n"
                      "\u00a7r---------------------------\n"
                      "\u00a7rYour Ping: %s%dms\n"
                      "\u00a7rCurrent DIM: %s%s",
                      level_name.c_str(),
                      server_version.c_str(),
                      get_tps_color(tps), tps_int, tps_frac, tick_usage,
                      get_mspt_color(mspt_avg), mspt_avg, get_mspt_color(mspt_cur), mspt_cur,
                      overworld_chunks, nether_chunks, end_chunks,
                      get_entity_color(entity_count), entity_count,
                      get_ping_color(ping), ping,
                      get_dim_color(player_dim), player_dim.c_str());

        player->sendTip(buf);
    }

    static bool cmd_monitor(PrimeBDS &plugin, endstone::CommandSender &sender,
                            const std::vector<std::string> &args)
    {
        auto *player = dynamic_cast<endstone::Player *>(&sender);
        if (!player)
        {
            sender.sendMessage("\u00a7cOnly players can use this command.");
            return true;
        }

        std::string player_name = player->getName();
        std::string mode = (!args.empty()) ? args[0] : "server";

        // If already monitoring, cancel existing
        auto it = plugin.monitor_intervals.find(player_name);
        if (it != plugin.monitor_intervals.end())
        {
            auto &scheduler = plugin.getServer().getScheduler();
            scheduler.cancelTask(it->second);
            plugin.monitor_intervals.erase(it);

            if (mode == "disable" || args.empty())
            {
                sender.sendMessage("\u00a7cMonitoring turned off");
                return true;
            }
            sender.sendMessage("\u00a7ePrevious monitoring canceled, applying new settings...");
        }
        else if (mode == "disable")
        {
            sender.sendMessage("\u00a7cMonitoring is not active.");
            return true;
        }

        // Schedule repeating task (every 20 ticks = 1 second)
        auto task = plugin.getServer().getScheduler().runTaskTimer(
            plugin,
            [&plugin, player_name]()
            {
                monitor_tick(plugin, player_name);
            },
            0, 20);

        if (task)
        {
            plugin.monitor_intervals[player_name] = task->getTaskId();
            sender.sendMessage("\u00a7aMonitoring turned on (server mode)");
        }
        else
        {
            sender.sendMessage("\u00a7cFailed to start monitoring task.");
        }

        return true;
    }

    REGISTER_COMMAND(monitor, "Monitor server performance in real time!", cmd_monitor,
                     info.usages = {"/monitor (server|disable)"};
                     info.permissions = {"primebds.command.monitor"};);
} // namespace primebds::commands
