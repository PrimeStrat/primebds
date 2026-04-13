#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"

#include <ctime>
#include <map>

namespace primebds::commands
{

    static std::map<std::string, double> warp_cooldowns;

    static bool cmd_warp(PrimeBDS &plugin, endstone::CommandSender &sender,
                         const std::vector<std::string> &args)
    {
        auto *player = dynamic_cast<endstone::Player *>(&sender);
        if (!player)
        {
            sender.sendMessage("\u00a7cOnly players can use this command.");
            return true;
        }

        if (args.empty())
        {
            // List warps as default
            auto warps = plugin.serverdb->getAllWarps();
            if (warps.empty())
            {
                sender.sendMessage("\u00a7cThere are no warps set");
                return true;
            }
            sender.sendMessage("\u00a7aWarps:");
            for (auto &w : warps)
            {
                std::string display = w.displayname.empty() ? w.name : w.displayname;
                sender.sendMessage("\u00a77- \u00a7b" + display);
            }
            return true;
        }

        std::string sub = args[0];
        for (auto &c : sub)
            c = (char)std::tolower(c);

        if (sub == "list")
        {
            auto warps = plugin.serverdb->getAllWarps();
            if (warps.empty())
            {
                sender.sendMessage("\u00a7cThere are no warps set");
                return true;
            }
            sender.sendMessage("\u00a7aWarps:");
            for (auto &w : warps)
            {
                std::string display = w.displayname.empty() ? w.name : w.displayname;
                std::string line = "\u00a7b" + display;
                if (!w.description.empty())
                    line += " \u00a77- " + w.description;
                sender.sendMessage("\u00a77- " + line);
            }
            return true;
        }

        // Try to warp by name
        auto warp = plugin.serverdb->getWarp(sub);
        if (!warp)
        {
            sender.sendMessage("\u00a7cWarp \u00a7e" + sub + " \u00a7cdoes not exist");
            return true;
        }

        auto pos = db::ServerDB::decodeLocation(warp->pos);
        double x = pos["x"].get<double>();
        double y = pos["y"].get<double>();
        double z = pos["z"].get<double>();

        // Check cooldown
        double now = (double)std::time(nullptr);
        bool exempt = player->hasPermission("primebds.exempt.warp.cooldowns");
        auto it = warp_cooldowns.find(player->getXuid());
        if (!exempt && it != warp_cooldowns.end() && now - it->second < warp->cooldown)
        {
            double rem = warp->cooldown - (now - it->second);
            char buf[64];
            std::snprintf(buf, sizeof(buf), "\u00a7cYou must wait %.1fs before using this warp", rem);
            sender.sendMessage(buf);
            return false;
        }

        player->performCommand("tp " + std::to_string(x) + " " + std::to_string(y) + " " + std::to_string(z));
        std::string display = warp->displayname.empty() ? warp->name : warp->displayname;
        player->sendMessage("\u00a7aWarped to \u00a7e" + display);
        warp_cooldowns[player->getXuid()] = now;
        return true;
    }

    REGISTER_COMMAND(warp, "Warp to a warp location!", cmd_warp,
                     info.usages = {
                         "/warp",
                         "/warp <name: message>",
                         "/warp (list)"};
                     info.permissions = {"primebds.command.warp"};);
} // namespace primebds::commands
