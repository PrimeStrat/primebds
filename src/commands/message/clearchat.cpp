#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"

namespace primebds::commands
{

    static bool cmd_clearchat(PrimeBDS &plugin, endstone::CommandSender &sender,
                              const std::vector<std::string> &args)
    {
        std::string clear;
        for (int i = 0; i < 100; ++i)
            clear += "\n";

        for (auto *p : plugin.getServer().getOnlinePlayers())
        {
            p->sendMessage(clear);
        }
        plugin.getServer().broadcastMessage("\u00a7eChat has been cleared by \u00a7b" + sender.getName());
        return true;
    }

    REGISTER_COMMAND(clearchat, "Clears the chat for all players!", cmd_clearchat,
                     info.usages = {"/clearchat"};
                     info.permissions = {"primebds.command.clearchat"};
                     info.aliases = {"cc"};);
} // namespace primebds::commands
