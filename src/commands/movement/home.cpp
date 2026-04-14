#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"

#include <ctime>
#include <map>

namespace primebds::commands
{

    static std::map<std::string, double> home_cooldowns;

    static bool cmd_home(PrimeBDS &plugin, endstone::CommandSender &sender,
                         const std::vector<std::string> &args)
    {
        auto *player = sender.asPlayer();
        if (!player)
        {
            sender.sendMessage("\u00a7cOnly players can use this command.");
            return true;
        }

        std::string sub = (!args.empty()) ? args[0] : "";
        // Normalize sub to lowercase
        for (auto &c : sub)
            c = (char)std::tolower(c);

        auto homes = plugin.serverdb->getAllHomes(player->getName(), player->getXuid());

        // Default: warp to first home
        if (sub.empty())
        {
            if (homes.empty())
            {
                sender.sendMessage("\u00a7cYou have no homes set");
                return true;
            }
            auto &first = homes.begin()->second;
            auto pos = db::ServerDB::decodeLocation(first.pos);
            player->performCommand("tp " + std::to_string(pos["x"].get<double>()) + " " +
                                   std::to_string(pos["y"].get<double>()) + " " +
                                   std::to_string(pos["z"].get<double>()));
            player->sendMessage("\u00a7aWarped to \u00a7e" + homes.begin()->first);
            return true;
        }

        if (sub == "set")
        {
            std::string name = (args.size() >= 2) ? args[1] : "Home";
            auto loc = player->getLocation();
            std::string pos_json = db::ServerDB::encodeLocation(
                loc.getX(), loc.getY(), loc.getZ(),
                player->getDimension().getName(), loc.getPitch(), loc.getYaw());

            if (plugin.serverdb->createHome(player->getXuid(), player->getName(), name, pos_json))
            {
                sender.sendMessage("\u00a7aHome \u00a7e" + name + " \u00a7aset!");
            }
            else
            {
                sender.sendMessage("\u00a7cHome \u00a7e" + name + " \u00a7calready exists");
            }
            return true;
        }

        if (sub == "list")
        {
            if (homes.empty())
            {
                sender.sendMessage("\u00a7cYou have no homes");
                return true;
            }
            sender.sendMessage("\u00a7aYour homes:");
            for (auto &[name, home] : homes)
            {
                sender.sendMessage("\u00a77- \u00a7b" + name);
            }
            return true;
        }

        if (sub == "warp" && args.size() >= 2)
        {
            std::string name = args[1];
            auto it = homes.find(name);
            if (it == homes.end())
            {
                sender.sendMessage("\u00a7cHome \u00a7e" + name + " \u00a7cdoes not exist");
                return true;
            }
            auto pos = db::ServerDB::decodeLocation(it->second.pos);
            player->performCommand("tp " + std::to_string(pos["x"].get<double>()) + " " +
                                   std::to_string(pos["y"].get<double>()) + " " +
                                   std::to_string(pos["z"].get<double>()));
            player->sendMessage("\u00a7aWarped to \u00a7e" + name);
            home_cooldowns[player->getXuid()] = (double)std::time(nullptr);
            return true;
        }

        // FIX: Accept both "del" and "delete"
        if ((sub == "del" || sub == "delete") && args.size() >= 2)
        {
            std::string name = args[1];
            if (plugin.serverdb->deleteHome(name, player->getName(), player->getXuid()))
            {
                sender.sendMessage("\u00a7e" + name + " \u00a7adeleted");
            }
            else
            {
                sender.sendMessage("\u00a7e" + name + " \u00a7cdoes not exist");
            }
            return true;
        }

        if (sub == "max")
        {
            sender.sendMessage("\u00a7aYou currently have \u00a7e" + std::to_string(homes.size()) + " \u00a7ahomes");
            return true;
        }

        sender.sendMessage("\u00a7cInvalid usage. /home [set/list/warp/del/delete]");
        return false;
    }

    REGISTER_COMMAND(home, "Manage your homes!", cmd_home,
                     info.usages = {
                         "/home",
                         "/home (set) [name: string]",
                         "/home (list)",
                         "/home (warp) <name: string>",
                         "/home (del|delete) <name: string>",
                         "/home (max)"};
                     info.permissions = {"primebds.command.home"};);

} // namespace primebds::commands
