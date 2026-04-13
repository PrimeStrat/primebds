#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"
#include "primebds/utils/target_selector.h"

namespace primebds::commands
{

    static bool cmd_bossbar(PrimeBDS &plugin, endstone::CommandSender &sender,
                            const std::vector<std::string> &args)
    {
        if (args.empty())
        {
            sender.sendMessage("\u00a7cUsage: /bossbar <set|clear> [player] [text] [color] [percent]");
            return false;
        }

        std::string sub = args[0];
        for (auto &c : sub)
            c = (char)std::tolower(c);

        if (sub == "clear")
        {
            if (args.size() >= 2)
            {
                auto targets = utils::getMatchingActors(plugin.getServer(), args[1], sender);
                for (auto *t : targets)
                {
                    if (auto *p = dynamic_cast<endstone::Player *>(t))
                    {
                        p->sendTitle("", "");
                    }
                }
                sender.sendMessage("\u00a7aCleared bossbar for " + std::to_string(targets.size()) + " player(s)");
            }
            else
            {
                auto *player = dynamic_cast<endstone::Player *>(&sender);
                if (player)
                    player->sendTitle("", "");
                sender.sendMessage("\u00a7aBossbar cleared");
            }
            return true;
        }

        if (sub == "set")
        {
            if (args.size() < 3)
            {
                sender.sendMessage("\u00a7cUsage: /bossbar set <player> <text> [color] [percent]");
                return false;
            }

            auto targets = utils::getMatchingActors(plugin.getServer(), args[1], sender);
            std::string text = args[2];

            for (auto *t : targets)
            {
                if (auto *p = dynamic_cast<endstone::Player *>(t))
                {
                    p->sendTitle(text, "", 10, 70, 20);
                }
            }
            sender.sendMessage("\u00a7aBossbar set for \u00a7e" + std::to_string(targets.size()) + " \u00a7aplayer(s)");
            return true;
        }

        sender.sendMessage("\u00a7cUnknown subcommand. Use /bossbar <set|clear>");
        return false;
    }

    REGISTER_COMMAND(bossbar, "Displays a client-sided bossbar!", cmd_bossbar,
                     info.usages = {
                         "/bossbar (set) <player: player> <text: message> [color: string] [percent: float]",
                         "/bossbar (clear) [player: player]"};
                     info.permissions = {"primebds.command.bossbar"};);

} // namespace primebds::commands
