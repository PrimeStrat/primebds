#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"

namespace primebds::commands
{

    static bool cmd_spectate(PrimeBDS &plugin, endstone::CommandSender &sender,
                             const std::vector<std::string> &args)
    {
        auto *player = dynamic_cast<endstone::Player *>(&sender);
        if (!player)
        {
            sender.sendMessage("\u00a7cOnly players can use this command.");
            return true;
        }

        // Collect spectatable players (not in spectator, not matching excluded tags)
        std::vector<endstone::Player *> candidates;
        for (auto *p : plugin.getServer().getOnlinePlayers())
        {
            if (p == player)
                continue;
            if (p->getGameMode() == endstone::GameMode::Spectator)
                continue;
            candidates.push_back(p);
        }

        if (candidates.empty())
        {
            sender.sendMessage("\u00a7cNo players available to spectate");
            return true;
        }

        if (!args.empty())
        {
            // Direct player spec
            std::string target_name = args[0];
            for (auto *c : candidates)
            {
                if (c->getName() == target_name)
                {
                    auto loc = c->getLocation();
                    player->performCommand("tp " + std::to_string(loc.getX()) + " " +
                                           std::to_string(loc.getY()) + " " +
                                           std::to_string(loc.getZ()));
                    player->sendMessage("\u00a7aSpectating \u00a7e" + c->getName());
                    return true;
                }
            }
            sender.sendMessage("\u00a7cPlayer \u00a7e" + target_name + " \u00a7cnot found or not spectatable");
            return true;
        }

        // List spectatable players
        sender.sendMessage("\u00a7a--- Available Players ---");
        for (auto *c : candidates)
        {
            sender.sendMessage("\u00a77- \u00a7e" + c->getName());
        }
        sender.sendMessage("\u00a77Use /spectate <player> to warp");
        return true;
    }

    REGISTER_COMMAND(spectate, "Warp to a player to spectate them!", cmd_spectate,
                     info.usages = {
                         "/spectate",
                         "/spectate <player: player>"};
                     info.permissions = {"primebds.command.spectate"};);
} // namespace primebds::commands
