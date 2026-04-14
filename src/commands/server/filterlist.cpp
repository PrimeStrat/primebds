#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <set>

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
            c = static_cast<char>(std::tolower(c));
        int page = (args.size() >= 2) ? std::atoi(args[1].c_str()) : 1;
        if (page < 1)
            page = 1;
        const int per_page = 10;

        std::vector<std::string> results;

        if (filter == "ops")
        {
            // Read permissions.json from server root for operator entries
            auto perms_path = plugin.getDataFolder().parent_path().parent_path() / "permissions.json";
            if (!std::filesystem::exists(perms_path))
            {
                sender.sendMessage("\u00a7cpermissions.json not found");
                return true;
            }

            try
            {
                std::ifstream f(perms_path);
                auto data = nlohmann::json::parse(f);
                std::vector<std::string> op_xuids;
                for (auto &entry : data)
                {
                    if (entry.value("permission", "") == "operator")
                        op_xuids.push_back(entry.value("xuid", ""));
                }

                for (auto &xuid : op_xuids)
                {
                    auto user = plugin.db->getUserByXuid(xuid);
                    std::string name = user ? user->name : "\u00a78Unknown";
                    results.push_back("\u00a7e" + name + " \u00a78(" + xuid + ")");
                }

                std::sort(results.begin(), results.end(), [](const std::string &a, const std::string &b)
                          {
                bool a_unknown = a.find("Unknown") != std::string::npos;
                bool b_unknown = b.find("Unknown") != std::string::npos;
                if (a_unknown != b_unknown)
                    return !a_unknown;
                return a < b; });
            }
            catch (const std::exception &e)
            {
                sender.sendMessage("\u00a7cFailed to read permissions.json: " + std::string(e.what()));
                return true;
            }
        }
        else if (filter == "default")
        {
            // All users with internal_rank "default" from DB
            auto all_users = plugin.db->getAllUsers();
            for (auto &u : all_users)
            {
                std::string rank = u.internal_rank;
                for (auto &c : rank)
                    c = static_cast<char>(std::tolower(c));
                if (rank == "default")
                    results.push_back(u.name);
            }
        }
        else if (filter == "online")
        {
            for (auto *p : plugin.getServer().getOnlinePlayers())
                results.push_back(p->getName());
        }
        else if (filter == "offline")
        {
            std::set<std::string> online_names;
            for (auto *p : plugin.getServer().getOnlinePlayers())
                online_names.insert(p->getName());

            auto all_users = plugin.db->getAllUsers();
            for (auto &u : all_users)
            {
                if (online_names.find(u.name) == online_names.end())
                    results.push_back(u.name);
            }
        }
        else if (filter == "muted")
        {
            // Validate each mute is still active via checkAndUpdateMute
            auto muted = plugin.db->getMutedUsers();
            for (auto &m : muted)
            {
                if (plugin.db->checkAndUpdateMute(m.xuid, m.name))
                    results.push_back(m.name);
            }
        }
        else if (filter == "banned")
        {
            // User bans from mod log
            auto banned = plugin.db->getBannedUsers();
            for (auto &u : banned)
                results.push_back(u.name + " \u00a77(User Banned)");

            // Name bans from server DB
            auto name_bans = plugin.serverdb->getAllNameBans();
            for (auto &nb : name_bans)
                results.push_back(nb.name + " \u00a77(Name Banned)");
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
            sender.sendMessage("\u00a77No " + filter + " players found");
            return true;
        }

        int total = static_cast<int>(results.size());
        int total_pages = (total + per_page - 1) / per_page;
        if (page > total_pages)
        {
            sender.sendMessage("\u00a7cInvalid page number. Available pages: 1-" + std::to_string(total_pages));
            return false;
        }

        int start = (page - 1) * per_page;
        int end = std::min(start + per_page, total);

        std::string msg = "\u00a7r" + filter + " Players \u00a77(Page " +
                          std::to_string(page) + "/" + std::to_string(total_pages) + "):";
        for (int i = start; i < end; ++i)
            msg += "\n\u00a77- \u00a7e" + results[i];

        sender.sendMessage(msg);
        return true;
    }

    REGISTER_COMMAND(filterlist, "Lists all players with a filter!", cmd_filterlist,
                     info.usages = {"/filterlist (ops|default|online|offline|muted|banned|ipbanned)<plist_filter: plist_filter> [page: int]"};
                     info.permissions = {"primebds.command.filterlist"};
                     info.aliases = {"flist"};);

} // namespace primebds::commands
