/// @file back.cpp
/// Teleport to your last saved location!

#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"

#include <ctime>
#include <map>

namespace primebds::commands {

    static bool cmd_back(PrimeBDS &, endstone::CommandSender &,
                        const std::vector<std::string> &);

    REGISTER_COMMAND(back, "Teleport to your last saved location!", cmd_back,
                     info.usages = {"/back"};
                     info.permissions = {"primebds.command.back"};);

    static std::map<std::string, double> back_cooldowns;

    /// Teleport to your last saved location!
    static bool cmd_back(PrimeBDS &plugin, endstone::CommandSender &sender,
                         const std::vector<std::string> &args) {
        auto *player = sender.asPlayer();
        if (!player) {
            sender.sendMessage("\u00a7cOnly players can use this command.");
            return true;
        }

        auto user = plugin.db->getOnlineUser(player->getXuid());
        if (!user || user->last_logout_pos.empty()) {
            sender.sendMessage("\u00a7cNo previous location found");
            return true;
        }

        bool exempt = player->hasPermission("primebds.exempt.back");

        // Check cooldown
        double now = (double)std::time(nullptr);
        auto it = back_cooldowns.find(player->getXuid());
        if (!exempt && it != back_cooldowns.end()) {
            double diff = now - it->second;
            if (diff < 5.0) {
                sender.sendMessage("\u00a7cYou must wait before using /back again");
                return true;
            }
        }

        // Parse stored position "x,y,z,dim"
        std::string pos_str = user->last_logout_pos;
        auto p1 = pos_str.find(',');
        auto p2 = pos_str.find(',', p1 + 1);
        auto p3 = pos_str.find(',', p2 + 1);
        if (p1 == std::string::npos || p2 == std::string::npos) {
            sender.sendMessage("\u00a7cSaved position data is invalid");
            return true;
        }

        // Teleport via command for dimension safety
        std::string x = pos_str.substr(0, p1);
        std::string y = pos_str.substr(p1 + 1, p2 - p1 - 1);
        std::string z = pos_str.substr(p2 + 1, p3 != std::string::npos ? p3 - p2 - 1 : std::string::npos);

        player->performCommand("tp " + x + " " + y + " " + z);
        player->sendMessage("\u00a7aTeleported to your last location");
        back_cooldowns[player->getXuid()] = now;
        return true;
    }

} // namespace primebds::commands
