/// @file home.cpp
/// Manage and warp to homes!

#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"

#include <algorithm>
#include <ctime>
#include <map>
#include <string>

namespace primebds::commands {

    static bool cmd_home(PrimeBDS &, endstone::CommandSender &,
                        const std::vector<std::string> &);

    REGISTER_COMMAND(home, "Manage and warp to homes!", cmd_home,
                     info.usages = {
                         "/home",
                         "/home (set) [name: string]",
                         "/home (list)",
                         "/home (warp) <name: string>",
                         "/home (del|delete) <name: string>",
                         "/home (max)"};
                     info.permissions = {"primebds.command.home"};);

    static std::map<std::string, double> home_cooldowns;

    /// Parse the max home count from a player's permissions (primebds.homes.N).
    /// Returns -1 for unlimited (primebds.homes.exempt).
    static int getMaxHomes(endstone::Player *player) {
        if (player->hasPermission("primebds.homes.exempt"))
            return -1;

        int max_homes = 1;
        for (auto *perm : player->getEffectivePermissions()) {
            auto name = perm->getPermission();
            if (name.rfind("primebds.homes.", 0) == 0 && name != "primebds.homes.exempt") {
                try {
                    int n = std::stoi(name.substr(name.rfind('.') + 1));
                    max_homes = std::max(max_homes, n);
                } catch (...) {
                }
            }
        }
        return max_homes;
    }

    /// Manage and warp to homes!
    static bool cmd_home(PrimeBDS &plugin, endstone::CommandSender &sender,
                         const std::vector<std::string> &args) {
        auto *player = sender.asPlayer();
        if (!player) {
            sender.sendMessage("\u00a7cOnly players can use this command.");
            return true;
        }

        auto settings = plugin.serverdb->getHomeSettings();
        bool exempt_cooldown = player->hasPermission("primebds.exempt.home.cooldowns");

        double now = static_cast<double>(std::time(nullptr));
        double last_used = home_cooldowns.count(player->getXuid()) ? home_cooldowns[player->getXuid()] : 0;

        if (!exempt_cooldown && settings.cooldown > 0 && (now - last_used) < settings.cooldown) {
            double remaining = settings.cooldown - (now - last_used);
            char buf[64];
            std::snprintf(buf, sizeof(buf), "%.1f", remaining);
            sender.sendMessage("\u00a7cYou must wait " + std::string(buf) + "s before using /home again");
            return true;
        }

        std::string sub = (!args.empty()) ? args[0] : "";
        for (auto &c : sub)
            c = static_cast<char>(std::tolower(c));

        auto homes = plugin.serverdb->getAllHomes(player->getName(), player->getXuid());

        // Default: warp to first home
        if (sub.empty()) {
            if (homes.empty()) {
                sender.sendMessage("\u00a7cYou have no homes set");
                return true;
            }
            auto &first = homes.begin()->second;
            auto pos = db::ServerDB::decodeLocation(first.pos);
            player->teleport(endstone::Location(
                player->getDimension(),
                pos["x"].get<double>(), pos["y"].get<double>(), pos["z"].get<double>()));
            player->sendMessage("\u00a7aWarped to \u00a7e" + homes.begin()->first);
            home_cooldowns[player->getXuid()] = now;
            return true;
        }

        if (sub == "set") {
            std::string name = (args.size() >= 2) ? args[1] : "Home";

            int max_homes = getMaxHomes(player);
            if (max_homes >= 0 && static_cast<int>(homes.size()) >= max_homes) {
                sender.sendMessage("\u00a7cYou can only have " + std::to_string(max_homes) + " homes");
                return true;
            }

            auto loc = player->getLocation();
            std::string pos_json = db::ServerDB::encodeLocation(
                loc.getX(), loc.getY(), loc.getZ(),
                player->getDimension().getName(), loc.getPitch(), loc.getYaw());

            if (plugin.serverdb->createHome(player->getXuid(), player->getName(), name, pos_json)) {
                sender.sendMessage("\u00a7e" + name + " \u00a7aset successfully at your current location");
            } else {
                sender.sendMessage("\u00a7e" + name + " \u00a7calready exists");
            }
            return true;
        }

        if (sub == "list") {
            if (homes.empty()) {
                sender.sendMessage("\u00a7cYou have no homes set");
                return true;
            }
            std::string msg = "\u00a7aYour homes:\n";
            for (auto &[name, home] : homes) {
                msg += "\u00a77- \u00a7b" + name + "\n";
            }
            sender.sendMessage(msg);
            return true;
        }

        if (sub == "warp" && args.size() >= 2) {
            std::string name = args[1];
            auto it = homes.find(name);
            if (it == homes.end()) {
                sender.sendMessage("\u00a7cHome \u00a7e" + name + " \u00a7cdoes not exist");
                return true;
            }
            auto pos = db::ServerDB::decodeLocation(it->second.pos);
            player->teleport(endstone::Location(
                player->getDimension(),
                pos["x"].get<double>(), pos["y"].get<double>(), pos["z"].get<double>()));
            player->sendMessage("\u00a7aWarped to \u00a7e" + name);
            home_cooldowns[player->getXuid()] = now;
            return true;
        }

        if ((sub == "del" || sub == "delete") && args.size() >= 2) {
            std::string name = args[1];
            if (plugin.serverdb->deleteHome(name, player->getName(), player->getXuid())) {
                sender.sendMessage("\u00a7e" + name + " \u00a7adeleted");
            } else {
                sender.sendMessage("\u00a7e" + name + " \u00a7cdoes not exist");
            }
            return true;
        }

        if (sub == "max") {
            int max_homes = getMaxHomes(player);
            std::string max_str = (max_homes < 0) ? "unlimited" : std::to_string(max_homes);
            sender.sendMessage("\u00a7aYou can have up to \u00a7e" + max_str + " \u00a7ahomes");
            return true;
        }

        sender.sendMessage("\u00a7cInvalid usage. /home [set/list/warp/del/delete/max]");
        return false;
    }


} // namespace primebds::commands
