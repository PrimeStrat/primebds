/// @file ping.cpp
/// Check your or another player's latency!

#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"
#include "primebds/utils/target_selector.h"

namespace primebds::commands {

    static bool cmd_ping(PrimeBDS &, endstone::CommandSender &,
                        const std::vector<std::string> &);

    REGISTER_COMMAND(ping, "Check your or another player's latency!", cmd_ping,
                     info.usages = {"/ping [player: player]"};
                     info.permissions = {"primebds.command.ping"};);

    static const char *get_ping_color(int ping) {
        if (ping <= 80)
            return "\u00a7a";
        if (ping <= 160)
            return "\u00a7e";
        return "\u00a7c";
    }

    /// Check your or another player's latency!
    static bool cmd_ping(PrimeBDS &plugin, endstone::CommandSender &sender,
                         const std::vector<std::string> &args) {
        if (args.empty()) {
            auto *player = sender.asPlayer();
            if (!player) {
                sender.sendMessage("\u00a7cThis command can only be executed by a player");
                return false;
            }
            int ping = static_cast<int>(player->getPing().count());
            sender.sendMessage(std::string("Your ping is ") + get_ping_color(ping) +
                               std::to_string(ping) + "\u00a7rms");
            return true;
        }

        auto targets = utils::getMatchingActors(plugin.getServer(), args[0], sender);

        if (!targets.empty()) {
            if (targets.size() == 1) {
                auto *p = dynamic_cast<endstone::Player *>(targets[0]);
                if (p) {
                    int ping = static_cast<int>(p->getPing().count());
                    sender.sendMessage("The ping of " + p->getName() + " is " +
                                       get_ping_color(ping) + std::to_string(ping) + "\u00a7rms");
                }
            } else {
                std::string msg = "\u00a7bMatched Players' Pings:\n";
                for (auto *t : targets) {
                    auto *p = dynamic_cast<endstone::Player *>(t);
                    if (p) {
                        int ping = static_cast<int>(p->getPing().count());
                        msg += "\u00a77- \u00a7r" + p->getName() + ": " +
                               get_ping_color(ping) + std::to_string(ping) + "\u00a7rms\n";
                    }
                }
                sender.sendMessage(msg);
            }
            return true;
        }

        // Check offline user if not a selector
        if (args[0][0] != '@') {
            auto offline = plugin.db->getUserByName(args[0]);
            if (offline) {
                int last_ping = offline->ping;
                if (last_ping >= 0) {
                    sender.sendMessage(offline->name + " is offline. Last recorded ping: " +
                                       get_ping_color(last_ping) + std::to_string(last_ping) + "\u00a7rms");
                } else {
                    sender.sendMessage(offline->name + " is offline, and their ping data is unavailable");
                }
                return true;
            }
        }

        sender.sendMessage("\u00a7cNo matching players found for '" + args[0] + "'.");
        return true;
    }

} // namespace primebds::commands
