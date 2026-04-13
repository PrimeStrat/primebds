#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"
#include "primebds/utils/target_selector.h"

#include <algorithm>
#include <string>

namespace primebds::commands
{

    static bool cmd_itemtag(PrimeBDS &plugin, endstone::CommandSender &sender,
                            const std::vector<std::string> &args)
    {
        if (args.size() < 3)
        {
            sender.sendMessage("\u00a7cUsage: /itemtag <player> <unbreakable> <true|false>");
            return false;
        }
        auto targets = utils::getMatchingActors(plugin.getServer(), args[0], sender);
        std::string tag_type = args[1];
        std::transform(tag_type.begin(), tag_type.end(), tag_type.begin(), ::tolower);
        std::string val_str = args[2];
        std::transform(val_str.begin(), val_str.end(), val_str.begin(), ::tolower);
        bool value = (val_str == "true" || val_str == "1" || val_str == "yes" || val_str == "on");

        for (auto *t : targets)
        {
            auto *p = dynamic_cast<endstone::Player *>(t);
            if (!p)
                continue;
            auto held = p->getInventory().getItemInMainHand();
            if (!held || held->getType() == endstone::ItemType::Air)
                continue;
            auto meta = held->getItemMeta();
            if (tag_type == "unbreakable")
                meta->setUnbreakable(value);
            held->setItemMeta(meta.get());
            p->getInventory().setItem(p->getInventory().getHeldItemSlot(), *held);
            std::string status = value ? "\u00a7aTrue" : "\u00a7cFalse";
            sender.sendMessage("\u00a7e" + p->getName() + "\u00a7r's item tag \u00a7eUnbreakable \u00a7rset to " + status);
        }
        return true;
    }

    REGISTER_COMMAND(itemtag, "Modify item tags!", cmd_itemtag,
                     info.usages = {"/itemtag <player: player> (unbreakable) <value: bool>"};
                     info.permissions = {"primebds.command.itemtag"};);
} // namespace primebds::commands
