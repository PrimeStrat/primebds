#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"

namespace primebds::commands
{

    static bool cmd_iteminfo(PrimeBDS &plugin, endstone::CommandSender &sender,
                             const std::vector<std::string> &args)
    {
        auto *player = sender.asPlayer();
        if (!player)
        {
            sender.sendMessage("\u00a7cThis command can only be executed by a player");
            return false;
        }

        auto held = player->getInventory().getItemInMainHand();
        if (!held || held->getType() == endstone::ItemType::Air)
        {
            sender.sendMessage("\u00a7cYou are not holding an item");
            return false;
        }

        auto meta = held->getItemMeta();
        std::string msg = "\u00a7bItem Info:\n";
        msg += "\u00a77- \u00a7eType: \u00a7f" + std::string(held->getType().getId()) + "\n";
        msg += "\u00a77- \u00a7eAmount: \u00a7f" + std::to_string(held->getAmount()) + "\n";
        if (!meta->getDisplayName().empty())
            msg += "\u00a77- \u00a7eDisplay Name: \u00a7r" + meta->getDisplayName() + "\n";
        msg += "\u00a77- \u00a7eDamage: \u00a7r" + std::to_string(meta->getDamage()) + "\n";
        msg += "\u00a77- \u00a7eUnbreakable: \u00a7r" + std::string(meta->isUnbreakable() ? "true" : "false") + "\n";

        auto lore = meta->getLore();
        if (!lore.empty())
        {
            msg += "\u00a77- \u00a7eLore:\n";
            for (size_t i = 0; i < lore.size(); ++i)
                msg += "  \u00a77- \u00a77" + std::to_string(i + 1) + ". \u00a7b" + lore[i] + "\n";
        }

        auto enchants = meta->getEnchants();
        if (!enchants.empty())
        {
            msg += "\u00a77- \u00a7eEnchantments:\n";
            for (auto &[ench, lvl] : enchants)
                msg += "  \u00a77- \u00a7d" + std::string(ench->getId()) + " \u00a7r" + std::to_string(lvl) + "\n";
        }

        player->sendMessage(msg);
        return true;
    }

    REGISTER_COMMAND(iteminfo, "Check item data!", cmd_iteminfo,
                     info.usages = {"/iteminfo"};
                     info.permissions = {"primebds.command.iteminfo"};);
} // namespace primebds::commands
