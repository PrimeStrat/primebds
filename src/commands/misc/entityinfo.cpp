#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"

#include <cmath>
#include <map>
#include <algorithm>

namespace primebds::commands
{

    static bool cmd_entityinfo(PrimeBDS &plugin, endstone::CommandSender &sender,
                               const std::vector<std::string> &args)
    {
        for (auto &a : args)
            if (a.find('@') != std::string::npos)
            {
                sender.sendMessage("\u00a7cTarget selectors are invalid for this command");
                return false;
            }

        auto actors = plugin.getServer().getLevel()->getActors();

        if (args.empty())
        {
            auto *player = dynamic_cast<endstone::Player *>(&sender);
            if (!player)
            {
                sender.sendMessage("\u00a7cOnly players can use this command.");
                return true;
            }

            // Look at nearest entity via raycast
            sender.sendMessage("\u00a7eTotal entities loaded: \u00a7f" + std::to_string(actors.size()));
            return true;
        }

        if (!args.empty() && args[0] == "list")
        {
            int page = (args.size() > 1) ? std::max(1, std::atoi(args[1].c_str())) : 1;
            std::map<std::string, int> counts;
            for (auto *e : actors)
                counts[e->getType()]++;

            sender.sendMessage("\u00a7eEntity Summary \u00a77(Page " + std::to_string(page) + "):");
            sender.sendMessage("\u00a7eTotal: \u00a7f" + std::to_string(actors.size()));
            for (auto &[type, count] : counts)
            {
                sender.sendMessage("  \u00a77- \u00a7e" + type + ": \u00a7f" + std::to_string(count));
            }
            return true;
        }

        return true;
    }

    REGISTER_COMMAND(entityinfo, "Check entity information!", cmd_entityinfo,
                     info.usages = {"/entityinfo (list) [page: int]"};
                     info.permissions = {"primebds.command.entityinfo"};);
} // namespace primebds::commands
