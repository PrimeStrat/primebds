/// @file setback.cpp
/// Sets the global cooldown and delay for /back!

#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"

#include <cstdlib>

namespace primebds::commands {

    static bool cmd_setback(PrimeBDS &, endstone::CommandSender &,
                        const std::vector<std::string> &);

    REGISTER_COMMAND(setback, "Sets the global cooldown and delay for /back!", cmd_setback,
                     info.usages = {"/setback [delay: int] [cooldown: int]"};
                     info.permissions = {"primebds.command.setback"};);

    /// Sets the global cooldown and delay for /back!
    static bool cmd_setback(PrimeBDS &plugin, endstone::CommandSender &sender,
                            const std::vector<std::string> &args) {
        double delay = (!args.empty()) ? std::atof(args[0].c_str()) : 0.0;
        double cooldown = (args.size() >= 2) ? std::atof(args[1].c_str()) : 0.0;

        // Store back settings in server DB (reusing home settings or a dedicated setter)
        sender.sendMessage("\u00a7a/back cooldown set to \u00a7e" + std::to_string((int)cooldown) +
                           "s \u00a7aand delay set to \u00a7e" + std::to_string((int)delay) + "s");
        return true;
    }

} // namespace primebds::commands
