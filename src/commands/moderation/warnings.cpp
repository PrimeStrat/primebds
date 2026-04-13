#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"
#include "primebds/utils/moderation.h"

#include <algorithm>
#include <cmath>
#include <ctime>
#include <cstdlib>

namespace primebds::commands
{

    static bool cmd_warnings(PrimeBDS &plugin, endstone::CommandSender &sender,
                             const std::vector<std::string> &args)
    {
        if (args.empty())
        {
            sender.sendMessage("\u00a7cUsage: /warnings <player> [page]");
            return false;
        }

        std::string target_name = args[0];
        auto user = plugin.db->getUserByName(target_name);
        if (!user)
        {
            sender.sendMessage("\u00a7cPlayer not found");
            return false;
        }

        // Check for delete/clear actions first
        if (args.size() >= 2)
        {
            std::string action = args[1];
            if (action == "delete" && args.size() >= 3)
            {
                int id = std::atoi(args[2].c_str());
                plugin.db->removeWarning(id);
                sender.sendMessage("\u00a76Warning \u00a7eID " + std::to_string(id) + " \u00a76was erased");
                return true;
            }
            if (action == "clear")
            {
                auto warns = plugin.db->getWarnings(user->xuid);
                for (auto &w : warns)
                    plugin.db->removeWarning(w.id);
                sender.sendMessage("\u00a76All warnings for \u00a7e" + target_name + " \u00a76were erased");
                return true;
            }
        }

        int page = (args.size() >= 2) ? std::max(1, std::atoi(args[1].c_str())) : 1;
        auto warnings = plugin.db->getWarnings(user->xuid);
        if (warnings.empty())
        {
            sender.sendMessage("\u00a76No warnings for \u00a7e" + target_name);
            return true;
        }

        int per_page = 5;
        int total_pages = std::max(1, (int)std::ceil((double)warnings.size() / per_page));
        page = std::min(page, total_pages);
        int start = (page - 1) * per_page;
        int end = std::min(start + per_page, (int)warnings.size());

        sender.sendMessage("\u00a76Warnings for \u00a7e" + target_name +
                           " \u00a77(Page " + std::to_string(page) + "/" + std::to_string(total_pages) + "):");
        for (int i = start; i < end; ++i)
        {
            auto &w = warnings[i];
            std::string expires = utils::formatTimeRemaining(w.warn_time);
            sender.sendMessage("\u00a78[\u00a77" + std::to_string(w.id) + "\u00a78] \u00a7f\"" +
                               w.warn_reason + "\" \u00a77- \u00a7e" + w.added_by +
                               " \u00a78[\u00a7e" + expires + "\u00a78]");
        }
        return true;
    }

    REGISTER_COMMAND(warnings, "List or delete warnings for a player!", cmd_warnings,
                     info.usages = {
                         "/warnings <player: player> [page: int]",
                         "/warnings <player: player> (delete|clear) [id: int]"};
                     info.permissions = {"primebds.command.warnings"};);
} // namespace primebds::commands
