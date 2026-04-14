/// @file punishments.cpp
/// View a player's punishment history!

#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"
#include "primebds/utils/moderation.h"

#include <algorithm>
#include <cmath>

namespace primebds::commands {

    static bool cmd_punishments(PrimeBDS &, endstone::CommandSender &,
                        const std::vector<std::string> &);

    REGISTER_COMMAND(punishments, "View a player's punishment history!", cmd_punishments,
                     info.usages = {"/punishments <player: player> [page: int]"};
                     info.permissions = {"primebds.command.punishments"};);

    /// View a player's punishment history!
    static bool cmd_punishments(PrimeBDS &plugin, endstone::CommandSender &sender,
                                const std::vector<std::string> &args) {
        if (args.empty()) {
            sender.sendMessage("\u00a7cUsage: /punishments <player> [page]");
            return false;
        }

        for (auto &a : args)
            if (a.find('@') != std::string::npos) {
                sender.sendMessage("\u00a7cTarget selectors are invalid for this command");
                return false;
            }

        std::string target_name = args[0];
        int page = (args.size() > 1) ? std::max(1, std::atoi(args[1].c_str())) : 1;

        auto user = plugin.db->getUserByName(target_name);
        if (!user) {
            sender.sendMessage("\u00a7cPlayer not found");
            return false;
        }

        auto history = plugin.db->getPunishmentHistory(user->xuid);
        if (history.empty()) {
            sender.sendMessage("\u00a7eNo punishment history for " + target_name);
            return true;
        }

        int per_page = 5;
        int total_pages = std::max(1, (int)std::ceil((double)history.size() / per_page));
        page = std::min(page, total_pages);
        int start = (page - 1) * per_page;
        int end = std::min(start + per_page, (int)history.size());

        sender.sendMessage("\u00a76Punishment History for \u00a7e" + target_name +
                           " \u00a77(Page " + std::to_string(page) + "/" + std::to_string(total_pages) + "):");

        for (int i = start; i < end; ++i) {
            auto &p = history[i];
            sender.sendMessage("\u00a78[\u00a77" + std::to_string(p.id) + "\u00a78] \u00a7e" +
                               p.action_type + " \u00a77- \u00a7f\"" + p.reason + "\"");
        }
        return true;
    }

} // namespace primebds::commands
