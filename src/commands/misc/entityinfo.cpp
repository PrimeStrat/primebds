/// @file entityinfo.cpp
/// Check entity information!

#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"

#include <cmath>
#include <map>
#include <algorithm>

namespace primebds::commands {

    static bool cmd_entityinfo(PrimeBDS &, endstone::CommandSender &,
                        const std::vector<std::string> &);

    REGISTER_COMMAND(entityinfo, "Check entity information!", cmd_entityinfo,
                     info.usages = {"/entityinfo (list)<action: entity_action> [page: int]"};
                     info.permissions = {"primebds.command.entityinfo"};);

    /// Check entity information!
    static bool cmd_entityinfo(PrimeBDS &plugin, endstone::CommandSender &sender,
                               const std::vector<std::string> &args) {
        for (auto &a : args)
            if (a.find('@') != std::string::npos) {
                sender.sendMessage("\u00a7cTarget selectors are invalid for this command");
                return false;
            }

        auto actors = plugin.getServer().getLevel()->getActors();

        if (args.empty()) {
            auto *player = sender.asPlayer();
            if (!player) {
                sender.sendMessage("\u00a7cOnly players can use this command.");
                return true;
            }

            // Look at nearest entity via raycast
            sender.sendMessage("\u00a7eTotal entities loaded: \u00a7f" + std::to_string(actors.size()));
            return true;
        }

        if (!args.empty() && args[0] == "list") {
            int page = (args.size() > 1) ? std::max(1, std::atoi(args[1].c_str())) : 1;
            std::map<std::string, int> counts;
            for (auto *e : actors)
                counts[e->getType()]++;

            // Sort by count descending
            std::vector<std::pair<std::string, int>> sorted_types(counts.begin(), counts.end());
            std::sort(sorted_types.begin(), sorted_types.end(),
                      [](const auto &a, const auto &b) { return a.second > b.second; });

            int per_page = 10;
            int total_pages = std::max(1, (int)std::ceil((double)sorted_types.size() / per_page));
            page = std::min(page, total_pages);
            int start = (page - 1) * per_page;
            int end = std::min(start + per_page, (int)sorted_types.size());

            sender.sendMessage("\u00a7eEntity Summary \u00a77(Page " + std::to_string(page) +
                               "/" + std::to_string(total_pages) + "):");
            sender.sendMessage("\u00a7eTotal: \u00a7f" + std::to_string(actors.size()));
            for (int i = start; i < end; ++i) {
                sender.sendMessage("  \u00a77- \u00a7e" + sorted_types[i].first +
                                   ": \u00a7f" + std::to_string(sorted_types[i].second));
            }
            return true;
        }

        return true;
    }

} // namespace primebds::commands
