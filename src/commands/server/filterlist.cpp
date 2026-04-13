#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"

#include <algorithm>

namespace primebds::commands
{

    static bool cmd_filterlist(PrimeBDS &plugin, endstone::CommandSender &sender,
                               const std::vector<std::string> &args)
    {
        if (args.empty())
        {
            sender.sendMessage("\u00a7cUsage: /filterlist <ops|default|online|offline|muted|banned|ipbanned> [page]");
            return false;
        }

        std::string filter = args[0];
        for (auto &c : filter)
            c = (char)std::tolower(c);
        int page = (args.size() >= 2) ? std::atoi(args[1].c_str()) : 1;
        if (page < 1)
            page = 1;
        const int per_page = 10;

        std::vector<std::string> results;

        if (filter == "ops")
        {
            for (auto *p : plugin.getServer().getOnlinePlayers())
            {
                if (p->isOp())
                    results.push_back(p->getName());
            }
        }
        else if (filter == "default")
        {
            for (auto *p : plugin.getServer().getOnlinePlayers())
            {
                if (!p->isOp())
                    results.push_back(p->getName());
            }
        }
        else if (filter == "online")
        {
            for (auto *p : plugin.getServer().getOnlinePlayers())
            {
                results.push_back(p->getName());
            }
        }
        else if (filter == "offline")
        {
            auto all_users = plugin.db->getAllUsers();
            for (auto &u : all_users)
            {
                if (!plugin.getServer().getPlayer(u.name))
                    results.push_back(u.name);
            }
        }
        else if (filter == "muted")
        {
            auto muted = plugin.db->getMutedUsers();
            for (auto &u : muted)
                results.push_back(u.name);
        }
        else if (filter == "banned")
        {
            auto banned = plugin.db->getBannedUsers();
            for (auto &u : banned)
                results.push_back(u.name);
        }
        else if (filter == "ipbanned")
        {
            auto ipbanned = plugin.db->getIPBannedUsers();
            for (auto &u : ipbanned)
                results.push_back(u.name);
        }
        else
        {
            sender.sendMessage("\u00a7cInvalid filter. Valid: ops, default, online, offline, muted, banned, ipbanned");
            return false;
        }

        if (results.empty())
        {
            sender.sendMessage("\u00a7cNo players found for filter: " + filter);
            return true;
        }

        int total_pages = ((int)results.size() + per_page - 1) / per_page;
        if (page > total_pages)
            page = total_pages;

        int start = (page - 1) * per_page;
        int end = std::min(start + per_page, (int)results.size());

        sender.sendMessage("\u00a7a--- " + filter + " (" + std::to_string(results.size()) +
                           " total) - Page " + std::to_string(page) + "/" + std::to_string(total_pages) + " ---");
        for (int i = start; i < end; ++i)
        {
            sender.sendMessage("\u00a77" + std::to_string(i + 1) + ". \u00a7e" + results[i]);
        }
        return true;
    }

    REGISTER_COMMAND(filterlist, "List players by filter!", cmd_filterlist,
                     info.usages = {"/filterlist <ops|default|online|offline|muted|banned|ipbanned> [page: int]"};
                     info.permissions = {"primebds.command.filterlist"};
                     info.aliases = {"flist"};);
} // namespace primebds::commands
