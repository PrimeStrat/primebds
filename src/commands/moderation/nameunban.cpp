#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"
#include "primebds/utils/logging.h"

namespace primebds::commands
{

    static bool cmd_nameunban(PrimeBDS &plugin, endstone::CommandSender &sender,
                              const std::vector<std::string> &args)
    {
        if (args.empty())
        {
            sender.sendMessage("\u00a7cUsage: /nameunban <name>");
            return false;
        }

        std::string name = args[0];
        if (!plugin.serverdb->checkNameBan(name))
        {
            sender.sendMessage("\u00a76Name \u00a7e" + name + " \u00a76is not banned");
            return false;
        }

        plugin.serverdb->removeNameBan(name);
        sender.sendMessage("\u00a76Name \u00a7e" + name + " \u00a76has been unbanned");
        utils::log(plugin.getServer(), "\u00a76Name \u00a7e" + name + " \u00a76was unbanned by \u00a7e" + sender.getName(), "mod");
        return true;
    }

    REGISTER_COMMAND(nameunban, "Unbans a player name from the server!", cmd_nameunban,
                     info.usages = {"/nameunban <name: string>"};
                     info.permissions = {"primebds.command.nameunban"};);
} // namespace primebds::commands
