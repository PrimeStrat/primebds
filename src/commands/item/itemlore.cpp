#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"
#include "primebds/utils/target_selector.h"

#include <algorithm>
#include <string>

namespace primebds::commands
{

    static bool cmd_itemlore(PrimeBDS &plugin, endstone::CommandSender &sender,
                             const std::vector<std::string> &args)
    {
        if (args.size() < 2)
        {
            sender.sendMessage("\u00a7cUsage: /itemlore <player> <add|set|delete|clear> [message|line]");
            return false;
        }
        auto targets = utils::getMatchingActors(plugin.getServer(), args[0], sender);
        if (targets.empty())
        {
            sender.sendMessage("\u00a7cNo matching players found");
            return false;
        }

        std::string action = args[1];
        std::transform(action.begin(), action.end(), action.begin(), ::tolower);
        std::string extra = (args.size() > 2) ? args[2] : "";

        for (auto *t : targets)
        {
            auto *p = dynamic_cast<endstone::Player *>(t);
            if (!p)
                continue;
            auto held = p->getInventory().getItemInMainHand();
            if (!held || held->getType() == endstone::ItemType::Air)
                continue;
            auto meta = held->getItemMeta();
            auto lore = meta->getLore();

            if (action == "add" && !extra.empty())
            {
                lore.push_back(extra);
                meta->setLore(lore);
            }
            else if (action == "set" && !extra.empty())
            {
                meta->setLore(std::vector<std::string>{extra});
            }
            else if (action == "delete")
            {
                if (!lore.empty())
                {
                    if (!extra.empty())
                    {
                        int idx = std::stoi(extra) - 1;
                        if (idx >= 0 && idx < static_cast<int>(lore.size()))
                            lore.erase(lore.begin() + idx);
                    }
                    else
                    {
                        lore.pop_back();
                    }
                    meta->setLore(lore);
                }
            }
            else if (action == "clear")
            {
                meta->setLore(std::nullopt);
            }
            else
            {
                sender.sendMessage("\u00a7cInvalid action. Use add, set, delete, or clear");
                return false;
            }

            held->setItemMeta(meta.get());
            p->getInventory().setItem(p->getInventory().getHeldItemSlot(), *held);
        }

        sender.sendMessage("\u00a7e" + std::to_string(targets.size()) + " \u00a7rplayers' item lores were updated");
        return true;
    }

    REGISTER_COMMAND(itemlore, "Modify item lore data!", cmd_itemlore,
                     info.usages = {"/itemlore <player: player> (add|set|delete|clear) [text: string]"};
                     info.permissions = {"primebds.command.itemlore"};
                     info.default_permission = "op";
                     info.aliases = {"lore"};);
} // namespace primebds::commands
