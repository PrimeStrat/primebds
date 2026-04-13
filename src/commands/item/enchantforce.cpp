#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"
#include "primebds/utils/target_selector.h"

#include <algorithm>
#include <string>

namespace primebds::commands
{

    static bool cmd_enchantforce(PrimeBDS &plugin, endstone::CommandSender &sender,
                                 const std::vector<std::string> &args)
    {
        if (args.size() < 2)
        {
            sender.sendMessage("\u00a7cUsage: /enchantforce <player> <enchantmentName> [level]");
            return false;
        }
        std::string enchant_name = args[1];
        std::transform(enchant_name.begin(), enchant_name.end(), enchant_name.begin(), ::tolower);

        int level = 1;
        if (args.size() >= 3)
        {
            try
            {
                level = std::stoi(args[2]);
            }
            catch (...)
            {
                sender.sendMessage("\u00a7cInvalid enchantment level; must be a number");
                return false;
            }
            if (level > 32767)
            {
                sender.sendMessage("\u00a7cEnchantments cannot exceed level 32767");
                return false;
            }
            if (level < -32767)
            {
                sender.sendMessage("\u00a7cEnchantments cannot be below level -32767");
                return false;
            }
        }

        auto targets = utils::getMatchingActors(plugin.getServer(), args[0], sender);
        if (targets.empty())
        {
            sender.sendMessage("\u00a7cNo matching players found");
            return false;
        }

        for (auto *t : targets)
        {
            auto *p = dynamic_cast<endstone::Player *>(t);
            if (!p)
                continue;
            auto held = p->getInventory().getItemInMainHand();
            if (!held || held->getType() == endstone::ItemType::Air)
                continue;
            auto meta = held->getItemMeta();
            meta->addEnchant(enchant_name, level, true);
            held->setItemMeta(meta.get());
            p->getInventory().setItem(p->getInventory().getHeldItemSlot(), *held);
        }

        if (targets.size() == 1)
        {
            auto *p = dynamic_cast<endstone::Player *>(targets[0]);
            sender.sendMessage("\u00a7e" + (p ? p->getName() : "Player") +
                               " \u00a7rwas enchanted with \u00a7e" + enchant_name +
                               " \u00a7rlevel \u00a7e" + std::to_string(level));
        }
        else
        {
            sender.sendMessage("\u00a7e" + std::to_string(targets.size()) +
                               " \u00a7rplayers were enchanted with \u00a7e" + enchant_name +
                               " \u00a7rlevel \u00a7e" + std::to_string(level));
        }
        return true;
    }

    REGISTER_COMMAND(enchantforce, "Forces a given enchantment onto an item!", cmd_enchantforce,
                     info.usages = {"/enchantforce <player: player> [enchantment: string] [level: int]"};
                     info.permissions = {"primebds.command.enchantforce"};
                     info.default_permission = "op";
                     info.aliases = {"enchantf", "forceenchant"};);
} // namespace primebds::commands
