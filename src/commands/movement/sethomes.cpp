#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"

#include <cstdlib>

namespace primebds::commands
{

    static bool cmd_sethomes(PrimeBDS &plugin, endstone::CommandSender &sender,
                             const std::vector<std::string> &args)
    {
        double delay = (!args.empty()) ? std::atof(args[0].c_str()) : 0.0;
        double cooldown = (args.size() >= 2) ? std::atof(args[1].c_str()) : 0.0;
        double cost = (args.size() >= 3) ? std::atof(args[2].c_str()) : 0.0;

        plugin.serverdb->setHomeSettings(delay, cooldown, cost);
        sender.sendMessage("\u00a7aGlobal home settings updated:\n"
                           "\u00a77- \u00a7eDelay: " +
                           std::to_string((int)delay) + "s\n"
                                                        "\u00a77- \u00a7eCooldown: " +
                           std::to_string((int)cooldown) + "s\n"
                                                           "\u00a77- \u00a7eCost: " +
                           std::to_string(cost));
        return true;
    }

    REGISTER_COMMAND(sethomes, "Set global home settings (delay, cooldown, cost)!", cmd_sethomes,
                     info.usages = {"/sethomes <delay: int> <cooldown: int> <cost: int>"};
                     info.permissions = {"primebds.command.sethomes"};);
} // namespace primebds::commands
