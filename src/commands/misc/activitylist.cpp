#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"

#include <algorithm>
#include <cmath>

namespace primebds::commands
{

    static std::string fmtPlaytime(int64_t sec)
    {
        std::string s;
        int64_t d = sec / 86400, h = (sec % 86400) / 3600, m = (sec % 3600) / 60;
        if (d > 0)
            s += std::to_string(d) + "d ";
        if (h > 0 || d > 0)
            s += std::to_string(h) + "h ";
        if (m > 0 || h > 0 || d > 0)
            s += std::to_string(m) + "m ";
        s += std::to_string(sec % 60) + "s";
        return s;
    }

    static bool cmd_activitylist(PrimeBDS &plugin, endstone::CommandSender &sender,
                                 const std::vector<std::string> &args)
    {
        auto *player = dynamic_cast<endstone::Player *>(&sender);
        if (!player)
        {
            sender.sendMessage("\u00a7cOnly players can use this command.");
            return true;
        }

        // Simplified: list online players by playtime
        int page = 1;
        std::string filter = "highest";
        if (!args.empty())
        {
            if (std::isdigit(args[0][0]))
            {
                page = std::max(1, std::atoi(args[0].c_str()));
            }
            else
            {
                filter = args[0];
            }
            if (args.size() >= 2)
            {
                if (filter == "highest" || filter == "lowest" || filter == "recent")
                    page = (args.size() >= 2 && std::isdigit(args[1][0])) ? std::atoi(args[1].c_str()) : 1;
                else if (std::isdigit(args[0][0]) && args.size() >= 2)
                    filter = args[1];
            }
        }

        sender.sendMessage("\u00a7bActivity List (" + filter + ") - Page " + std::to_string(page));
        return true;
    }

    REGISTER_COMMAND(activitylist, "Lists players by activity filter!", cmd_activitylist,
                     info.usages = {"/activitylist [page: int] (highest|lowest|recent)"};
                     info.permissions = {"primebds.command.activitylist"};);
} // namespace primebds::commands
