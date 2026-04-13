#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"
#include "primebds/utils/target_selector.h"

#include <cstdlib>
#include <cmath>

namespace primebds::commands
{

    static bool cmd_speed(PrimeBDS &plugin, endstone::CommandSender &sender,
                          const std::vector<std::string> &args)
    {
        if (args.empty())
        {
            sender.sendMessage("\u00a7cUsage: /speed <value> | /speed <flyspeed|walkspeed> <value> [player]");
            return false;
        }

        auto *self_player = dynamic_cast<endstone::Player *>(&sender);

        // Single arg = auto-detect fly/walk and set speed on self
        if (args.size() == 1)
        {
            if (!self_player)
            {
                sender.sendMessage("\u00a7cOnly players can use this shorthand");
                return false;
            }

            std::string arg = args[0];
            if (arg == "reset")
            {
                if (self_player->isFlying())
                {
                    self_player->setFlySpeed(0.05f);
                    sender.sendMessage("\u00a7bFlyspeed \u00a7rreset to default");
                }
                else
                {
                    self_player->setWalkSpeed(0.1f);
                    sender.sendMessage("\u00a7bWalkspeed \u00a7rreset to default");
                }
                return true;
            }

            float val = std::strtof(arg.c_str(), nullptr);
            if (self_player->isFlying())
            {
                sender.sendMessage("\u00a7e" + self_player->getName() + "'s \u00a7bflyspeed \u00a7rchanged to \u00a7e" + std::to_string(val));
                self_player->setFlySpeed(val);
            }
            else
            {
                sender.sendMessage("\u00a7e" + self_player->getName() + "'s \u00a7bwalkspeed \u00a7rchanged to \u00a7e" + std::to_string(val));
                self_player->setWalkSpeed(val);
            }
            return true;
        }

        // Check for reset subcommand
        if (args[0] == "reset")
        {
            if (args.size() < 2)
            {
                sender.sendMessage("\u00a7cUsage: /speed reset <flyspeed|walkspeed> [player]");
                return false;
            }
            std::string attr = args[1];
            auto targets = (args.size() >= 3) ? utils::getMatchingActors(plugin.getServer(), args[2], sender)
                                              : std::vector<endstone::Actor *>{self_player};
            for (auto *t : targets)
            {
                if (auto *p = dynamic_cast<endstone::Player *>(t))
                {
                    if (attr == "flyspeed")
                    {
                        p->setFlySpeed(0.05f);
                        p->sendMessage("\u00a7bFlyspeed \u00a7rreset to default");
                    }
                    else
                    {
                        p->setWalkSpeed(0.1f);
                        p->sendMessage("\u00a7bWalkspeed \u00a7rreset to default");
                    }
                }
            }
            return true;
        }

        // Normal: /speed <attr> <value> [player]
        std::string attr = args[0];
        if (attr != "flyspeed" && attr != "walkspeed")
        {
            sender.sendMessage("\u00a7cUnknown speed attribute: " + attr);
            return false;
        }

        float new_speed = std::strtof(args[1].c_str(), nullptr);
        auto targets = (args.size() >= 3) ? utils::getMatchingActors(plugin.getServer(), args[2], sender)
                                          : std::vector<endstone::Actor *>{self_player};

        for (auto *t : targets)
        {
            if (auto *p = dynamic_cast<endstone::Player *>(t))
            {
                if (attr == "flyspeed")
                    p->setFlySpeed(new_speed);
                else
                    p->setWalkSpeed(new_speed);
            }
        }

        if (targets.size() == 1)
        {
            auto *p = dynamic_cast<endstone::Player *>(targets[0]);
            if (p)
                sender.sendMessage("\u00a7e" + p->getName() + "'s \u00a7b" + attr + " \u00a7rchanged to \u00a7e" + std::to_string(new_speed));
        }
        else
        {
            sender.sendMessage("\u00a7e" + std::to_string(targets.size()) + " \u00a7rplayers had their \u00a7b" + attr + " \u00a7rchanged");
        }
        return true;
    }

    REGISTER_COMMAND(speed, "Modifies player flyspeed or walkspeed!", cmd_speed,
                     info.usages = {
                         "/speed <value: float>",
                         "/speed (flyspeed|walkspeed) <value: float> [player: player]",
                         "/speed (reset) (flyspeed|walkspeed) [player: player]"};
                     info.permissions = {"primebds.command.speed"};);

} // namespace primebds::commands
