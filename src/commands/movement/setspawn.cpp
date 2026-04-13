#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"

#include <cstdlib>

namespace primebds::commands
{

    static bool cmd_setspawn(PrimeBDS &plugin, endstone::CommandSender &sender,
                             const std::vector<std::string> &args)
    {
        auto *player = dynamic_cast<endstone::Player *>(&sender);
        if (!player)
        {
            sender.sendMessage("\u00a7cOnly players can use this command.");
            return true;
        }

        auto loc = player->getLocation();
        std::string pos_json = db::ServerDB::encodeLocation(
            loc.getX(), loc.getY(), loc.getZ(),
            player->getDimension().getName(), loc.getPitch(), loc.getYaw());

        plugin.serverdb->setSpawn(pos_json);

        char buf[128];
        std::snprintf(buf, sizeof(buf), "\u00a7aSpawn point set at \u00a7e%.2f %.2f %.2f \u00a77(\u00a7e%s\u00a77)",
                      loc.getX(), loc.getY(), loc.getZ(), player->getDimension().getName().c_str());
        sender.sendMessage(buf);
        return true;
    }

    REGISTER_COMMAND(setspawn, "Sets the global spawn point!", cmd_setspawn,
                     info.usages = {"/setspawn"};
                     info.permissions = {"primebds.command.setspawn"};);
} // namespace primebds::commands
