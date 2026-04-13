#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"

#include <string>

namespace primebds::commands
{

    static bool cmd_check(PrimeBDS &plugin, endstone::CommandSender &sender,
                          const std::vector<std::string> &args)
    {
        if (args.empty())
        {
            sender.sendMessage("\u00a7cUsage: /check <player> [info|mod|network|world]");
            return false;
        }

        std::string player_name = args[0];
        std::string filter = (args.size() > 1) ? args[1] : "info";

        auto *target = plugin.getServer().getPlayer(player_name);
        auto user = plugin.db->getUserByName(player_name);
        if (!user.has_value() && !target)
        {
            sender.sendMessage("Player \u00a7e" + player_name + "\u00a7c not found in database.");
            return false;
        }

        if (filter == "info")
        {
            std::string status = target ? "\u00a7aOnline" : "\u00a7cOffline";
            auto &u = user.value();
            std::string msg = "\u00a7bPlayer Database Information:\n";
            msg += "\u00a77- \u00a7eName: \u00a7f" + u.name + " \u00a78[" + status + "\u00a78]\n";
            msg += "\u00a77- \u00a7eXUID: \u00a7f" + u.xuid + "\n";
            msg += "\u00a77- \u00a7eInternal Rank: \u00a7f" + u.internal_rank + "\n";
            msg += "\u00a77- \u00a7eDevice OS: \u00a7f" + u.device_os + "\n";
            msg += "\u00a77- \u00a7eClient Version: \u00a7f" + u.client_ver + "\n";
            msg += "\u00a77- \u00a7ePing: \u00a7f" + std::to_string(u.ping) + "ms\n";
            sender.sendMessage(msg);
        }
        else if (filter == "mod")
        {
            auto mod = plugin.db->getModLog(user->xuid);
            std::string msg = "\u00a76Player Mod Information:\n";
            msg += "\u00a77- \u00a7eName: \u00a7f" + user->name + "\n";
            msg += "\u00a77- \u00a7eRank: \u00a7f" + user->internal_rank + "\n";
            if (mod.has_value())
            {
                msg += "\u00a77- \u00a7eBanned: \u00a7f" + std::string(mod->is_banned ? "true" : "false") + "\n";
                if (mod->is_banned)
                    msg += "\u00a77  - \u00a7eBan Reason: \u00a7f" + mod->ban_reason + "\n";
                msg += "\u00a77- \u00a7eMuted: \u00a7f" + std::string(mod->is_muted ? "true" : "false") + "\n";
                if (mod->is_muted)
                    msg += "\u00a77  - \u00a7eMute Reason: \u00a7f" + mod->mute_reason + "\n";
            }
            sender.sendMessage(msg);
        }
        else if (filter == "network")
        {
            std::string ip = target ? target->getAddress().getHostname() : "(offline)";
            sender.sendMessage("\u00a7dPlayer Network Information:\n\u00a77- \u00a7eIP: \u00a7f" + ip);
        }
        else if (filter == "world")
        {
            if (!target)
            {
                sender.sendMessage("\u00a7cPlayer must be online to check world data");
                return false;
            }
            std::string msg = "\u00a7aPlayer World Information:\n";
            msg += "\u00a77- \u00a7eGamemode: \u00a7f" + std::to_string(static_cast<int>(target->getGameMode())) + "\n";
            auto loc = target->getLocation();
            msg += "\u00a77- \u00a7eLocation: \u00a7f" + std::to_string(loc.getBlockX()) +
                   " / " + std::to_string(loc.getBlockY()) + " / " + std::to_string(loc.getBlockZ()) + "\n";
            msg += "\u00a77- \u00a7eDimension: \u00a7f" + target->getDimension().getName() + "\n";
            msg += "\u00a77- \u00a7eHealth: \u00a7f" + std::to_string(static_cast<int>(target->getHealth())) +
                   "/" + std::to_string(static_cast<int>(target->getMaxHealth())) + "\n";
            msg += "\u00a77- \u00a7eIs OP: \u00a7f" + std::string(target->isOp() ? "true" : "false") + "\n";
            sender.sendMessage(msg);
        }
        return true;
    }

    REGISTER_COMMAND(check, "Checks a player's client info!", cmd_check,
                     info.usages = {"/check <player: player> (info|mod|network|world)[info: info]"};
                     info.permissions = {"primebds.command.check"};
                     info.default_permission = "op";
                     info.aliases = {"seen"};);
} // namespace primebds::commands
