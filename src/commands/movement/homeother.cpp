#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"

#include <ctime>
#include <map>

namespace primebds::commands
{

    static std::map<std::string, double> homeother_cooldowns;

    static bool cmd_homeother(PrimeBDS &plugin, endstone::CommandSender &sender,
                              const std::vector<std::string> &args)
    {
        auto *player = sender.asPlayer();
        if (!player)
        {
            sender.sendMessage("\u00a7cOnly players can use this command.");
            return true;
        }

        if (args.empty())
        {
            sender.sendMessage("\u00a7cUsage: /homeother <player> [list/warp/set/del/max]");
            return false;
        }

        for (auto &a : args)
            if (a.find('@') != std::string::npos)
            {
                sender.sendMessage("\u00a7cTarget selectors are invalid for this command");
                return false;
            }

        std::string target_name = args[0];
        auto *target = plugin.getServer().getPlayer(target_name);
        if (!target)
        {
            sender.sendMessage("\u00a7cPlayer \u00a7e" + target_name + " \u00a7cnot found");
            return false;
        }

        std::string sub = (args.size() >= 2) ? args[1] : "";
        for (auto &c : sub)
            c = (char)std::tolower(c);

        auto homes = plugin.serverdb->getAllHomes(target->getName(), target->getXuid());

        if (sub.empty())
        {
            if (homes.empty())
            {
                sender.sendMessage("\u00a7c" + target_name + " has no homes");
                return true;
            }
            auto &first = homes.begin()->second;
            auto pos = db::ServerDB::decodeLocation(first.pos);
            player->performCommand("tp " + std::to_string(pos["x"].get<double>()) + " " +
                                   std::to_string(pos["y"].get<double>()) + " " +
                                   std::to_string(pos["z"].get<double>()));
            player->sendMessage("\u00a7aWarped to \u00a7e" + target_name + "'s " + homes.begin()->first);
            return true;
        }

        if (sub == "list")
        {
            if (homes.empty())
            {
                sender.sendMessage("\u00a7c" + target_name + " has no homes");
                return true;
            }
            sender.sendMessage("\u00a7a" + target_name + "'s homes:");
            for (auto &[name, h] : homes)
                sender.sendMessage("\u00a77- \u00a7b" + name);
            return true;
        }

        if (sub == "warp" && args.size() >= 3)
        {
            std::string name = args[2];
            auto it = homes.find(name);
            if (it == homes.end())
            {
                sender.sendMessage("\u00a7cHome \u00a7e" + name + " \u00a7cdoes not exist for \u00a7e" + target_name);
                return true;
            }
            auto pos = db::ServerDB::decodeLocation(it->second.pos);
            player->performCommand("tp " + std::to_string(pos["x"].get<double>()) + " " +
                                   std::to_string(pos["y"].get<double>()) + " " +
                                   std::to_string(pos["z"].get<double>()));
            player->sendMessage("\u00a7aWarped to \u00a7e" + target_name + "'s " + name);
            return true;
        }

        if (sub == "del" && args.size() >= 3)
        {
            std::string name = args[2];
            if (plugin.serverdb->deleteHome(name, target->getName(), target->getXuid()))
            {
                sender.sendMessage("\u00a7aDeleted \u00a7e" + target_name + "'s " + name);
            }
            else
            {
                sender.sendMessage("\u00a7c" + target_name + " does not have a home named \u00a7e" + name);
            }
            return true;
        }

        if (sub == "set")
        {
            std::string name = (args.size() >= 3) ? args[2] : "Home";
            auto loc = player->getLocation();
            std::string pos_json = db::ServerDB::encodeLocation(
                loc.getX(), loc.getY(), loc.getZ(),
                player->getDimension().getName(), loc.getPitch(), loc.getYaw());
            if (plugin.serverdb->createHome(target->getXuid(), target->getName(), name, pos_json))
            {
                sender.sendMessage("\u00a7aSet \u00a7e" + name + " \u00a7afor " + target_name);
            }
            else
            {
                sender.sendMessage("\u00a7c" + target_name + " already has a home named \u00a7e" + name);
            }
            return true;
        }

        if (sub == "max")
        {
            sender.sendMessage("\u00a7e" + target_name + " \u00a7ahas \u00a7e" + std::to_string(homes.size()) + " \u00a7ahomes");
            return true;
        }

        sender.sendMessage("\u00a7cInvalid usage. /homeother <player> [list/warp/set/del/max]");
        return false;
    }

    REGISTER_COMMAND(homeother, "Manage another player's homes!", cmd_homeother,
                     info.usages = {
                         "/homeother <player: player>",
                         "/homeother <player: player> (list)",
                         "/homeother <player: player> (warp) <name: string>",
                         "/homeother <player: player> (set) [name: string]",
                         "/homeother <player: player> (del) <name: string>",
                         "/homeother <player: player> (max)"};
                     info.permissions = {"primebds.command.homeother"};);

} // namespace primebds::commands
