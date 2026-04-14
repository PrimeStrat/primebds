/// @file warn.cpp
/// Warn a player that they are breaking a rule!

#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"
#include "primebds/utils/moderation.h"
#include "primebds/utils/logging.h"

#include <ctime>
#include <cstdlib>
#include <map>

namespace primebds::commands {

    static bool cmd_warn(PrimeBDS &, endstone::CommandSender &,
                        const std::vector<std::string> &);

    REGISTER_COMMAND(warn, "Warn a player that they are breaking a rule!", cmd_warn,
                     info.usages = {"/warn <player: player> <reason: string> [duration: int] [unit: string]"};
                     info.permissions = {"primebds.command.warn"};);

    static const std::map<std::string, int64_t> warn_time_units = {
        {"second", 1}, {"minute", 60}, {"hour", 3600}, {"day", 86400}, {"week", 604800}, {"month", 2592000}, {"year", 31536000}};

    /// Warn a player that they are breaking a rule!
    static bool cmd_warn(PrimeBDS &plugin, endstone::CommandSender &sender,
                         const std::vector<std::string> &args) {
        if (args.size() < 2) {
            sender.sendMessage("\u00a7cUsage: /warn <player> <reason> [duration] [unit]");
            return false;
        }

        std::string target_name = args[0];
        auto user = plugin.db->getUserByName(target_name);
        if (!user) {
            sender.sendMessage("\u00a7cPlayer not found");
            return false;
        }

        // Default permanent (200 years)
        int64_t duration_seconds = 200LL * 365 * 86400;
        size_t reason_end = args.size();

        // Check if last two args are duration + unit
        if (args.size() >= 4) {
            std::string maybe_unit = args[args.size() - 1];
            std::string maybe_num = args[args.size() - 2];
            bool is_num = !maybe_num.empty();
            for (char c : maybe_num)
                if (!std::isdigit(c)) {
                    is_num = false;
                    break;
                }

            if (is_num) {
                auto it = warn_time_units.find(maybe_unit);
                if (it != warn_time_units.end()) {
                    duration_seconds = std::atol(maybe_num.c_str()) * it->second;
                    reason_end = args.size() - 2;
                }
            }
        }

        std::string reason;
        for (size_t i = 1; i < reason_end; ++i) {
            if (i > 1)
                reason += " ";
            reason += args[i];
        }
        if (reason.empty()) {
            sender.sendMessage("\u00a7cProvide a reason");
            return false;
        }

        plugin.db->addWarning(user->xuid, user->name, reason, sender.getName());

        std::string expiration_str = utils::formatTimeRemaining(std::time(nullptr) + duration_seconds);
        sender.sendMessage("\u00a76Player \u00a7e" + target_name + " \u00a76was warned for \u00a7e\"" +
                           reason + "\" \u00a76which expires \u00a7e" + expiration_str);
        utils::log(plugin.getServer(), "\u00a76Player \u00a7e" + target_name + " \u00a76was warned by \u00a7e" + sender.getName() + " \u00a76for \u00a7e\"" + reason + "\"", "mod");
        return true;
    }

} // namespace primebds::commands
