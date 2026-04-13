/// @file crasher_patch.cpp
/// Crasher patch: blocks /me command with excessive @e selectors (DoS protection).

#include "primebds/handlers/preprocesses/crasher_patch.h"
#include "primebds/plugin.h"
#include "primebds/utils/config/config_manager.h"
#include "primebds/utils/logging.h"

#include <algorithm>
#include <string>

namespace primebds::handlers::preprocesses
{

    bool checkCrasherExploit(PrimeBDS &plugin, endstone::Player &player, const std::string &command)
    {
        // Count @e occurrences
        size_t count = 0;
        size_t pos = 0;
        while ((pos = command.find("@e", pos)) != std::string::npos)
        {
            ++count;
            pos += 2;
        }

        if (count < 5)
            return false;

        std::string xuid = player.getXuid();

        // Apply permission revocations if not already done
        if (plugin.crasher_patch_applied.find(xuid) == plugin.crasher_patch_applied.end())
        {
            player.addAttachment(plugin, "minecraft.command.me", false);
            player.addAttachment(plugin, "minecraft.command.tellraw", false);
            player.addAttachment(plugin, "minecraft.command.tell", false);
            player.addAttachment(plugin, "minecraft.command.w", false);
            player.addAttachment(plugin, "minecraft.command.msg", false);
            plugin.crasher_patch_applied.insert(xuid);
        }

        auto &cfg = config::ConfigManager::instance();
        auto conf = cfg.config();
        auto &modules = conf["modules"];

        bool enabled = modules.value("/me_crasher_patch/enabled"_json_pointer, true);
        if (!enabled)
            return true;

        bool ban_on_exploit = modules.value("/me_crasher_patch/ban"_json_pointer, false);
        if (ban_on_exploit)
        {
            (void)plugin.getServer().dispatchCommand(plugin.getServer().getCommandSender(),
                                                     "tempban " + player.getName() + " 7 day Crasher Exploit");
        }
        else
        {
            utils::log(plugin.getServer(),
                       "\u00a76Player \u00a7e" + player.getName() +
                           " \u00a76was kicked due to \u00a7eCrasher Exploit",
                       "mod");
            player.kick("Disconnected");
        }

        return true;
    }

} // namespace primebds::handlers::preprocesses
