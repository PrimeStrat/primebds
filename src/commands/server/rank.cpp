#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"

#include <algorithm>

namespace primebds::commands
{

    static bool cmd_rank(PrimeBDS &plugin, endstone::CommandSender &sender,
                         const std::vector<std::string> &args)
    {
        if (args.empty())
        {
            sender.sendMessage("\u00a7cUsage: /rank <set|create|delete|info|perm|list|inherit|weight|prefix|suffix> ...");
            return false;
        }

        std::string sub = args[0];
        for (auto &c : sub)
            c = (char)std::tolower(c);

        if (sub == "list")
        {
            auto ranks = plugin.serverdb->getAllRanks();
            if (ranks.empty())
            {
                sender.sendMessage("\u00a7cNo ranks exist");
                return true;
            }
            sender.sendMessage("\u00a7a--- Ranks ---");
            for (auto &r : ranks)
            {
                sender.sendMessage("\u00a7e" + r.name + " \u00a77[weight: " + std::to_string(r.weight) + "]" +
                                   (r.prefix.empty() ? "" : " \u00a77prefix: " + r.prefix));
            }
            return true;
        }

        if (sub == "create" && args.size() >= 2)
        {
            std::string name = args[1];
            if (plugin.serverdb->createRank(name))
            {
                sender.sendMessage("\u00a7aRank \u00a7e" + name + " \u00a7acreated");
            }
            else
            {
                sender.sendMessage("\u00a7cRank \u00a7e" + name + " \u00a7calready exists");
            }
            return true;
        }

        if (sub == "delete" && args.size() >= 2)
        {
            std::string name = args[1];
            if (plugin.serverdb->deleteRank(name))
            {
                sender.sendMessage("\u00a7aRank \u00a7e" + name + " \u00a7adeleted");
            }
            else
            {
                sender.sendMessage("\u00a7cRank \u00a7e" + name + " \u00a7cdoes not exist");
            }
            return true;
        }

        if (sub == "set" && args.size() >= 3)
        {
            std::string player_name = args[1];
            std::string rank_name = args[2];
            auto *target = plugin.getServer().getPlayer(player_name);
            if (!target)
            {
                sender.sendMessage("\u00a7cPlayer \u00a7e" + player_name + " \u00a7cnot found online");
                return false;
            }

            plugin.db->setUserRank(target->getXuid(), rank_name);
            sender.sendMessage("\u00a7e" + player_name + " \u00a7arank set to \u00a7e" + rank_name);
            return true;
        }

        if (sub == "info" && args.size() >= 2)
        {
            std::string name = args[1];
            auto rank = plugin.serverdb->getRank(name);
            if (!rank)
            {
                sender.sendMessage("\u00a7cRank \u00a7e" + name + " \u00a7cdoes not exist");
                return true;
            }
            sender.sendMessage("\u00a7a--- Rank: " + rank->name + " ---");
            sender.sendMessage("\u00a77Weight: \u00a7e" + std::to_string(rank->weight));
            if (!rank->prefix.empty())
                sender.sendMessage("\u00a77Prefix: \u00a7r" + rank->prefix);
            if (!rank->suffix.empty())
                sender.sendMessage("\u00a77Suffix: \u00a7r" + rank->suffix);
            if (!rank->inherits.empty())
                sender.sendMessage("\u00a77Inherits: \u00a7e" + rank->inherits);
            if (!rank->permissions.empty())
            {
                sender.sendMessage("\u00a77Permissions:");
                for (auto &p : rank->permissions)
                    sender.sendMessage("\u00a77- \u00a7b" + p);
            }
            return true;
        }

        if (sub == "perm" && args.size() >= 4)
        {
            std::string action = args[1];
            for (auto &c : action)
                c = (char)std::tolower(c);
            std::string rank_name = args[2];
            std::string perm = args[3];

            if (action == "add")
            {
                if (plugin.serverdb->addRankPermission(rank_name, perm))
                {
                    sender.sendMessage("\u00a7aPermission \u00a7e" + perm + " \u00a7aadded to rank \u00a7e" + rank_name);
                }
                else
                {
                    sender.sendMessage("\u00a7cFailed to add permission");
                }
            }
            else if (action == "remove")
            {
                if (plugin.serverdb->removeRankPermission(rank_name, perm))
                {
                    sender.sendMessage("\u00a7aPermission \u00a7e" + perm + " \u00a7aremoved from rank \u00a7e" + rank_name);
                }
                else
                {
                    sender.sendMessage("\u00a7cFailed to remove permission");
                }
            }
            return true;
        }

        if (sub == "inherit" && args.size() >= 3)
        {
            std::string name = args[1];
            std::string parent = args[2];
            plugin.serverdb->setRankInheritance(name, parent);
            sender.sendMessage("\u00a7aRank \u00a7e" + name + " \u00a7anow inherits from \u00a7e" + parent);
            return true;
        }

        if (sub == "weight" && args.size() >= 3)
        {
            std::string name = args[1];
            int weight = std::atoi(args[2].c_str());
            plugin.serverdb->setRankWeight(name, weight);
            sender.sendMessage("\u00a7aRank \u00a7e" + name + " \u00a7aweight set to \u00a7e" + std::to_string(weight));
            return true;
        }

        if (sub == "prefix" && args.size() >= 3)
        {
            std::string name = args[1];
            std::string prefix;
            for (size_t i = 2; i < args.size(); ++i)
            {
                if (i > 2)
                    prefix += " ";
                prefix += args[i];
            }
            plugin.serverdb->setRankPrefix(name, prefix);
            sender.sendMessage("\u00a7aRank \u00a7e" + name + " \u00a7aprefix set to \u00a7r" + prefix);
            return true;
        }

        if (sub == "suffix" && args.size() >= 3)
        {
            std::string name = args[1];
            std::string suffix;
            for (size_t i = 2; i < args.size(); ++i)
            {
                if (i > 2)
                    suffix += " ";
                suffix += args[i];
            }
            plugin.serverdb->setRankSuffix(name, suffix);
            sender.sendMessage("\u00a7aRank \u00a7e" + name + " \u00a7asuffix set to \u00a7r" + suffix);
            return true;
        }

        sender.sendMessage("\u00a7cInvalid subcommand for /rank");
        return false;
    }

    REGISTER_COMMAND(rank, "Manage server ranks!", cmd_rank,
                     info.usages = {
                         "/rank (set) <player: player> <rank: string>",
                         "/rank (create) <name: string>",
                         "/rank (delete) <name: string>",
                         "/rank (info) <name: string>",
                         "/rank (perm) <add|remove> <rank: string> <permission: message>",
                         "/rank (list)",
                         "/rank (inherit) <rank: string> <parent: string>",
                         "/rank (weight) <rank: string> <weight: int>",
                         "/rank (prefix) <rank: string> <prefix: message>",
                         "/rank (suffix) <rank: string> <suffix: message>"};
                     info.permissions = {"primebds.command.rank"};);

} // namespace primebds::commands
