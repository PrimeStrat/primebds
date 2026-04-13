#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"
#include "primebds/utils/target_selector.h"

namespace primebds::commands
{

    static bool cmd_toast(PrimeBDS &plugin, endstone::CommandSender &sender,
                          const std::vector<std::string> &args)
    {
        if (args.size() < 3)
        {
            sender.sendMessage("\u00a7cUsage: /toast <player> <title> <message>");
            return false;
        }

        std::string title = args[1];
        std::string msg;
        for (size_t i = 2; i < args.size(); ++i)
        {
            if (i > 2)
                msg += " ";
            msg += args[i];
        }

        auto targets = utils::getMatchingActors(plugin.getServer(), args[0], sender);
        for (auto *t : targets)
        {
            if (auto *p = dynamic_cast<endstone::Player *>(t))
            {
                p->sendToast(title, msg);
            }
        }
        sender.sendMessage("\u00a7aToast sent to " + std::to_string(targets.size()) + " player(s)");
        return true;
    }

    REGISTER_COMMAND(toast, "Sends a toast notification to a player!", cmd_toast,
                     info.usages = {"/toast <player: player> <title: string> <message: message>"};
                     info.permissions = {"primebds.command.toast"};);
} // namespace primebds::commands
