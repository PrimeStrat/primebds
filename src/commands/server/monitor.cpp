#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"

namespace primebds::commands
{

    static bool cmd_monitor(PrimeBDS &plugin, endstone::CommandSender &sender,
                            const std::vector<std::string> &args)
    {
        auto *player = dynamic_cast<endstone::Player *>(&sender);
        if (!player)
        {
            sender.sendMessage("\u00a7cOnly players can use this command.");
            return true;
        }

        std::string xuid = player->getXuid();

        // Toggle monitor on/off
        auto it = plugin.monitor_intervals.find(xuid);
        if (it != plugin.monitor_intervals.end())
        {
            // Disable
            plugin.monitor_intervals.erase(it);
            sender.sendMessage("\u00a7eServer monitor disabled");
            return true;
        }

        // Enable: start periodic display
        sender.sendMessage("\u00a7aServer monitor enabled");

        // Simple real-time stats snap
        auto &server = plugin.getServer();
        int online = (int)server.getOnlinePlayers().size();
        int max = server.getMaxPlayers();

        char buf[512];
        std::snprintf(buf, sizeof(buf),
                      "\u00a7b--- Server Monitor ---\n"
                      "\u00a77Players: \u00a7e%d/%d",
                      online, max);
        sender.sendMessage(buf);

        plugin.monitor_intervals[xuid] = 1;
        return true;
    }

    REGISTER_COMMAND(monitor, "Toggle server performance monitor!", cmd_monitor,
                     info.usages = {"/monitor"};
                     info.permissions = {"primebds.command.monitor"};);
} // namespace primebds::commands
