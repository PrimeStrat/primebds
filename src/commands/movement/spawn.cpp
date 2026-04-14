/// @file spawn.cpp
/// Warps you to the spawn!

#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"

#include <ctime>
#include <map>

namespace primebds::commands {

    static bool cmd_spawn(PrimeBDS &, endstone::CommandSender &,
                        const std::vector<std::string> &);

    REGISTER_COMMAND(spawn, "Warps you to the spawn!", cmd_spawn,
                     info.usages = {"/spawn"};
                     info.permissions = {"primebds.command.spawn"};);

    static std::map<std::string, double> spawn_cooldowns;

    /// Warps you to the spawn!
    static bool cmd_spawn(PrimeBDS &plugin, endstone::CommandSender &sender,
                          const std::vector<std::string> &args) {
        auto *player = sender.asPlayer();
        if (!player) {
            sender.sendMessage("\u00a7cOnly players can use this command.");
            return true;
        }

        auto spawn = plugin.serverdb->getSpawn();
        if (!spawn) {
            sender.sendMessage("\u00a7cNo spawn has been set yet");
            return false;
        }

        auto pos = db::ServerDB::decodeLocation(spawn->pos);
        double x = pos["x"].get<double>();
        double y = pos["y"].get<double>();
        double z = pos["z"].get<double>();

        double now = (double)std::time(nullptr);
        double cooldown_time = spawn->cooldown;
        auto it = spawn_cooldowns.find(player->getXuid());
        if (it != spawn_cooldowns.end() && now - it->second < cooldown_time) {
            double rem = cooldown_time - (now - it->second);
            char buf[64];
            std::snprintf(buf, sizeof(buf), "\u00a7cYou must wait %.1fs before using /spawn again", rem);
            sender.sendMessage(buf);
            return false;
        }

        player->performCommand("tp " + std::to_string(x) + " " + std::to_string(y) + " " + std::to_string(z));
        player->sendMessage("\u00a7aYou have been warped to spawn!");
        spawn_cooldowns[player->getXuid()] = now;
        return true;
    }

} // namespace primebds::commands
