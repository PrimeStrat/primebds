#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"
#include "primebds/utils/target_selector.h"

#include <algorithm>
#include <random>
#include <vector>

namespace primebds::commands
{

    static bool is_valid_spectate_target(endstone::Player *p)
    {
        if (!p)
            return false;
        if (p->getGameMode() == endstone::GameMode::Spectator)
            return false;
        return true;
    }

    static void warp_player(endstone::Player *sender, endstone::Player *target)
    {
        sender->setGameMode(endstone::GameMode::Spectator);
        sender->teleport(*target);
        sender->sendMessage("\u00a7aNow spectating \u00a7e" + target->getName());
    }

    static bool cmd_spectate(PrimeBDS &plugin, endstone::CommandSender &sender,
                             const std::vector<std::string> &args)
    {
        auto *player = sender.asPlayer();
        if (!player)
        {
            sender.sendMessage("\u00a7cOnly players can use this command.");
            return true;
        }

        // Block @a selector
        for (auto &arg : args)
        {
            if (arg.find("@a") != std::string::npos)
            {
                sender.sendMessage("\u00a7cYou cannot select all players for this command");
                return false;
            }
        }

        // Collect valid candidates
        std::vector<endstone::Player *> candidates;
        for (auto *p : plugin.getServer().getOnlinePlayers())
        {
            if (p == player)
                continue;
            if (!is_valid_spectate_target(p))
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
            // Direct target via selector or name
            auto targets = utils::getMatchingActors(plugin.getServer(), args[0], sender);
            if (targets.empty())
            {
                sender.sendMessage("\u00a7cUnable to find target player");
                return false;
            }

            // Shuffle and find first valid
            std::random_device rd;
            std::mt19937 rng(rd());
            std::shuffle(targets.begin(), targets.end(), rng);

            for (auto *t : targets)
            {
                auto *target = dynamic_cast<endstone::Player *>(t);
                if (target && is_valid_spectate_target(target) && target != player)
                {
                    warp_player(player, target);
                    return true;
                }
            }

            sender.sendMessage("\u00a7cNo valid players available to spectate.");
            return false;
        }

        // No args: show form UI
        endstone::ActionForm form;
        form.setTitle("Spectate Menu");
        form.setContent("Select a player to spectate!");

        for (auto *c : candidates)
        {
            form.addButton(c->getName());
        }

        form.setOnSubmit([candidates](endstone::Player *p, int selection)
                         {
            if (selection >= 0 && selection < static_cast<int>(candidates.size()))
            {
                auto *target = candidates[selection];
                if (target && is_valid_spectate_target(target))
                {
                    warp_player(p, target);
                }
                else
                {
                    p->sendMessage("\u00a7cThat player is no longer available.");
                }
            } });

        player->sendForm(std::move(form));
        return true;
    }

    REGISTER_COMMAND(spectate, "Warp to a player to spectate them!", cmd_spectate,
                     info.usages = {"/spectate [player: player]"};
                     info.permissions = {"primebds.command.spectate"};);
} // namespace primebds::commands
