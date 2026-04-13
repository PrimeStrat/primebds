#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"
#include "primebds/utils/moderation.h"
#include "primebds/utils/logging.h"

#include <ctime>

namespace primebds::commands
{

    static bool cmd_permban(PrimeBDS &plugin, endstone::CommandSender &sender,
                            const std::vector<std::string> &args)
    {
        if (args.empty())
        {
            sender.sendMessage("\u00a7cUsage: /permban <player> [reason]");
            return false;
        }

        for (auto &a : args)
            if (a.find('@') != std::string::npos)
            {
                sender.sendMessage("\u00a7cTarget selectors are invalid for this command");
                return false;
            }

        std::string target_name = args[0];
        std::string reason = "Negative Behavior";
        if (args.size() > 1)
        {
            reason.clear();
            for (size_t i = 1; i < args.size(); ++i)
            {
                if (i > 1)
                    reason += " ";
                reason += args[i];
            }
        }

        auto user = plugin.db->getUserByName(target_name);
        if (!user)
        {
            sender.sendMessage("\u00a7cPlayer not found");
            return false;
        }

        auto modlog = plugin.db->getModLog(user->xuid);
        if (modlog && modlog->is_banned)
        {
            sender.sendMessage("\u00a76Player \u00a7e" + target_name + " \u00a76is already banned");
            return false;
        }

        // Permanent = ~200 years
        int64_t expiration = std::time(nullptr) + (200LL * 365 * 86400);
        plugin.db->updateModLog(user->xuid, "is_banned", "1");
        plugin.db->updateModLog(user->xuid, "banned_time", std::to_string(expiration));
        plugin.db->updateModLog(user->xuid, "ban_reason", reason);

        std::string msg = utils::formatBanMessage(reason, expiration, plugin.getServer().getLevel()->getName());
        auto *target = plugin.getServer().getPlayer(target_name);
        if (target)
            target->kick(msg);

        sender.sendMessage("\u00a76Player \u00a7e" + target_name + " \u00a76was permanently banned for \u00a7e\"" + reason + "\"");
        utils::log(plugin.getServer(), "\u00a76Player \u00a7e" + target_name + " \u00a76was permanently banned by \u00a7e" + sender.getName(), "mod");
        return true;
    }

    REGISTER_COMMAND(permban, "Permanently bans a player from the server!", cmd_permban,
                     info.usages = {"/permban <player: player> [reason: message]"};
                     info.permissions = {"primebds.command.permban"};);
} // namespace primebds::commands
