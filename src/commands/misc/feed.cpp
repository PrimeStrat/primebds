#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"
#include "primebds/utils/target_selector.h"

namespace primebds::commands
{

    static bool cmd_feed(PrimeBDS &plugin, endstone::CommandSender &sender,
                         const std::vector<std::string> &args)
    {
        if (args.empty())
        {
            auto *player = dynamic_cast<endstone::Player *>(&sender);
            if (!player)
            {
                sender.sendMessage("This command can only be executed by a player");
                return false;
            }
            plugin.getServer().dispatchCommand(plugin.getServer().getCommandSender(),
                                               "effect " + player->getName() + " saturation 3 255 true");
            player->sendMessage("\u00a7aYou were fed");
            return true;
        }
        if (!sender.hasPermission("primebds.command.feed.other"))
        {
            sender.sendMessage("\u00a7cYou do not have permission to feed others");
            return true;
        }
        auto targets = utils::getMatchingActors(plugin.getServer(), args[0], sender);
        for (auto *t : targets)
        {
            auto *p = dynamic_cast<endstone::Player *>(t);
            if (p)
            {
                plugin.getServer().dispatchCommand(plugin.getServer().getCommandSender(),
                                                   "effect " + p->getName() + " saturation 3 255 true");
                p->sendMessage("\u00a7aYou were fed");
            }
        }
        sender.sendMessage("\u00a7e" + std::to_string(targets.size()) + " \u00a7rplayers were fed");
        return true;
    }

    REGISTER_COMMAND(feed, "Sets player hunger to full!", cmd_feed,
                     info.usages = {"/feed [player: player]"};
                     info.permissions = {"primebds.command.feed", "primebds.command.feed.other"};
                     info.default_permission = "op";
                     info.aliases = {"eat"};);
} // namespace primebds::commands
