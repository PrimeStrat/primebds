/// @file warps.cpp
/// Manage server warps!

#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"

#include <cstdlib>

namespace primebds::commands {

    static bool cmd_warps(PrimeBDS &, endstone::CommandSender &,
                        const std::vector<std::string> &);

    REGISTER_COMMAND(warps, "Manage server warps!", cmd_warps,
                     info.usages = {
                         "/warps",
                         "/warps (list)",
                         "/warps (create) <name: string> [displayname: string] [category: string] [description: string]",
                         "/warps (delete) <name: message>",
                         "/warps (addalias) <name: string> <alias: message>",
                         "/warps (removealias) <name: string> <alias: message>"};
                     info.permissions = {"primebds.command.warps"};);

    /// Manage server warps!
    static bool cmd_warps(PrimeBDS &plugin, endstone::CommandSender &sender,
                          const std::vector<std::string> &args) {
        auto *player = sender.asPlayer();
        if (!player) {
            sender.sendMessage("\u00a7cOnly players can use this command.");
            return true;
        }

        if (args.empty()) {
            // Open warps management â€” list all warps
            auto warps = plugin.serverdb->getAllWarps();
            if (warps.empty()) {
                sender.sendMessage("\u00a7cNo warps exist");
                return true;
            }
            sender.sendMessage("\u00a7aWarps:");
            for (auto &w : warps) {
                std::string display = w.displayname.empty() ? w.name : w.displayname;
                sender.sendMessage("\u00a77- \u00a7e" + display);
            }
            return true;
        }

        std::string sub = args[0];
        for (auto &c : sub)
            c = (char)std::tolower(c);

        if (sub == "list") {
            auto warps = plugin.serverdb->getAllWarps();
            if (warps.empty()) {
                sender.sendMessage("\u00a7cThere are no warps set");
                return true;
            }
            sender.sendMessage("\u00a7aWarps:");
            for (auto &w : warps) {
                std::string display = w.displayname.empty() ? w.name : w.displayname;
                std::string line = "\u00a7b" + display;
                if (!w.description.empty())
                    line += " \u00a77- " + w.description;
                sender.sendMessage("\u00a77- " + line);
            }
            return true;
        }

        if (sub == "create" && args.size() >= 2) {
            std::string name = args[1];
            std::string displayname = (args.size() >= 3) ? args[2] : "";
            std::string category = (args.size() >= 4) ? args[3] : "";
            std::string description = (args.size() >= 5) ? args[4] : "";
            double cost = (args.size() >= 6) ? std::atof(args[5].c_str()) : 0.0;
            double cooldown = (args.size() >= 7) ? std::atof(args[6].c_str()) : 0.0;
            double delay = (args.size() >= 8) ? std::atof(args[7].c_str()) : 0.0;

            auto loc = player->getLocation();
            std::string pos_json = db::ServerDB::encodeLocation(
                loc.getX(), loc.getY(), loc.getZ(),
                player->getDimension().getName(), loc.getPitch(), loc.getYaw());

            if (plugin.serverdb->createWarp(name, pos_json, displayname, category, description, cost, cooldown, delay)) {
                sender.sendMessage("\u00a7aWarp \u00a7e" + name + " \u00a7aset at your current location");
            } else {
                sender.sendMessage("\u00a7cWarp \u00a7e" + name + " \u00a7calready exists");
            }
            return true;
        }

        if (sub == "delete" && args.size() >= 2) {
            std::string name = args[1];
            if (plugin.serverdb->deleteWarp(name)) {
                sender.sendMessage("\u00a7aWarp \u00a7e" + name + " \u00a7adeleted");
            } else {
                sender.sendMessage("\u00a7cWarp \u00a7e" + name + " \u00a7cdoes not exist");
            }
            return true;
        }

        if (sub.substr(0, 3) == "set" && args.size() >= 3) {
            std::string name = args[1];
            auto warp = plugin.serverdb->getWarp(name);
            if (!warp) {
                sender.sendMessage("\u00a7cWarp \u00a7e" + name + " \u00a7cdoes not exist");
                return true;
            }

            if (sub == "setcost") {
                plugin.serverdb->updateWarpProperty(name, "cost", args[2]);
                sender.sendMessage("\u00a7aWarp \u00a7e" + name + " \u00a7acost updated");
            } else if (sub == "setdescription") {
                std::string val;
                for (size_t i = 2; i < args.size(); ++i) {
                    if (i > 2)
                        val += " ";
                    val += args[i];
                }
                plugin.serverdb->updateWarpProperty(name, "description", val);
                sender.sendMessage("\u00a7aWarp \u00a7e" + name + " \u00a7adescription updated");
            } else if (sub == "setcategory") {
                plugin.serverdb->updateWarpProperty(name, "category", args[2]);
                sender.sendMessage("\u00a7aWarp \u00a7e" + name + " \u00a7acategory updated");
            } else if (sub == "setdisplayname") {
                std::string val;
                for (size_t i = 2; i < args.size(); ++i) {
                    if (i > 2)
                        val += " ";
                    val += args[i];
                }
                plugin.serverdb->updateWarpProperty(name, "displayname", val);
                sender.sendMessage("\u00a7aWarp \u00a7e" + name + " \u00a7adisplay name updated");
            } else if (sub == "setdelay") {
                plugin.serverdb->updateWarpProperty(name, "delay", args[2]);
                sender.sendMessage("\u00a7aWarp \u00a7e" + name + " \u00a7adelay updated");
            } else if (sub == "setcooldown") {
                plugin.serverdb->updateWarpProperty(name, "cooldown", args[2]);
                sender.sendMessage("\u00a7aWarp \u00a7e" + name + " \u00a7acooldown updated");
            }
            return true;
        }

        if (sub == "addalias" && args.size() >= 3) {
            std::string warp_name = args[1];
            std::string alias;
            for (size_t i = 2; i < args.size(); ++i) {
                if (i > 2)
                    alias += " ";
                alias += args[i];
            }
            if (plugin.serverdb->addAlias(warp_name, alias)) {
                sender.sendMessage("\u00a7aAlias \u00a7e" + alias + " \u00a7aadded to warp \u00a7e" + warp_name);
            } else {
                sender.sendMessage("\u00a7cCould not add alias");
            }
            return true;
        }

        if (sub == "removealias" && args.size() >= 3) {
            std::string warp_name = args[1];
            std::string alias;
            for (size_t i = 2; i < args.size(); ++i) {
                if (i > 2)
                    alias += " ";
                alias += args[i];
            }
            if (plugin.serverdb->removeAlias(warp_name, alias)) {
                sender.sendMessage("\u00a7aAlias \u00a7e" + alias + " \u00a7aremoved from warp \u00a7e" + warp_name);
            } else {
                sender.sendMessage("\u00a7cCould not remove alias");
            }
            return true;
        }

        sender.sendMessage("\u00a7cInvalid usage. /warps [list/create/delete/set*/addalias/removealias]");
        return false;
    }


} // namespace primebds::commands
