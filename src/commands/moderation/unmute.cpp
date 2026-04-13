#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"
#include "primebds/utils/logging.h"

namespace primebds::commands
{

    static bool cmd_unmute(PrimeBDS &plugin, endstone::CommandSender &sender,
                           const std::vector<std::string> &args)
    {
        if (args.empty())
        {
            sender.sendMessage("\u00a7cUsage: /unmute <player>");
            return false;
        }

        for (auto &a : args)
            if (a.find('@') != std::string::npos)
            {
                sender.sendMessage("\u00a7cTarget selectors are invalid for this command");
                return false;
            }

        std::string target_name = args[0];
        auto user = plugin.db->getUserByName(target_name);
        if (!user)
        {
            sender.sendMessage("\u00a7cPlayer not found");
            return false;
        }

        auto modlog = plugin.db->getModLog(user->xuid);
        if (!modlog || !modlog->is_muted)
        {
            sender.sendMessage("\u00a76Player \u00a7e" + target_name + " \u00a76is not muted");
            return false;
        }

        plugin.db->updateModLog(user->xuid, "is_muted", "0");
        plugin.db->updateModLog(user->xuid, "mute_time", "0");
        plugin.db->updateModLog(user->xuid, "mute_reason", "");

        sender.sendMessage("\u00a76Player \u00a7e" + target_name + " \u00a76has been unmuted");
        utils::log(plugin.getServer(), "\u00a76Player \u00a7e" + target_name + " \u00a76was unmuted by \u00a7e" + sender.getName(), "mod");

        auto *target = plugin.getServer().getPlayer(target_name);
        if (target)
            target->sendMessage("\u00a76Your mute has expired!");
        return true;
    }

    REGISTER_COMMAND(unmute, "Removes an active mute from a player!", cmd_unmute,
                     info.usages = {"/unmute <player: player>"};
                     info.permissions = {"primebds.command.unmute"};);
} // namespace primebds::commands
