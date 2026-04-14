/// @file god.cpp
/// Toggles invulnerability!

#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"
#include "primebds/utils/target_selector.h"

namespace primebds::commands {

    static bool cmd_god(PrimeBDS &, endstone::CommandSender &,
                        const std::vector<std::string> &);

    REGISTER_COMMAND(god, "Toggles invulnerability!", cmd_god,
                     info.usages = {"/god [player: player] [toggle: bool]"};
                     info.permissions = {"primebds.command.god", "primebds.command.god.other"};
                     info.default_permission = "op";);

    /// Toggles invulnerability!
    static bool cmd_god(PrimeBDS &plugin, endstone::CommandSender &sender,
                        const std::vector<std::string> &args) {
        if (args.empty()) {
            auto *player = sender.asPlayer();
            if (!player) {
                sender.sendMessage("\u00a7cThis command can only be executed by a player");
                return false;
            }
            std::string id = std::to_string(player->getRuntimeId());
            if (plugin.isgod.count(id)) {
                plugin.isgod.erase(id);
                player->sendMessage("\u00a7cYou are no longer invulnerable");
            } else {
                plugin.isgod.insert(id);
                player->sendMessage("\u00a7aYou are now invulnerable");
            }
            return true;
        }
        if (!sender.hasPermission("primebds.command.god.other")) {
            sender.sendMessage("\u00a7cYou do not have permission to modify others' invulnerability");
            return true;
        }
        auto targets = utils::getMatchingActors(plugin.getServer(), args[0], sender);
        std::string force = (args.size() > 1) ? args[1] : "";
        for (auto *t : targets) {
            auto *p = dynamic_cast<endstone::Player *>(t);
            if (!p)
                continue;
            std::string id = std::to_string(p->getRuntimeId());
            bool enable;
            if (force == "true" || force == "on" || force == "1")
                enable = true;
            else if (force == "false" || force == "off" || force == "0")
                enable = false;
            else
                enable = !plugin.isgod.count(id);

            if (enable) {
                plugin.isgod.insert(id);
                p->sendMessage("\u00a7aYou are now invulnerable");
            } else {
                plugin.isgod.erase(id);
                p->sendMessage("\u00a7cYou are no longer invulnerable");
            }
        }
        sender.sendMessage("\u00a7eInvulnerability updated for " + std::to_string(targets.size()) + " players");
        return true;
    }

} // namespace primebds::commands
