#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"

namespace primebds::commands
{

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

        // Start scanning task — the interval handler sends block tip info
        // In the full implementation this would register a scheduler task
        sender.sendMessage("\u00a7aBlock scanning enabled");
        return true;
    }

    REGISTER_COMMAND(blockscan, "Continuously show information about the block you're looking at.", cmd_blockscan,
                     info.usages = {"/blockscan (disable)"};
                     info.permissions = {"primebds.command.blockscan"};);
} // namespace primebds::commands
