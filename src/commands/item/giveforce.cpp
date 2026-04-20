/// @file giveforce.cpp
/// Forces any item registered to be given, hidden or not!

#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"
#include "primebds/utils/target_selector.h"

#include <algorithm>
#include <string>

namespace primebds::commands {

    static bool cmd_giveforce(PrimeBDS &, endstone::CommandSender &,
                        const std::vector<std::string> &);

    REGISTER_COMMAND(giveforce, "Forces any item registered to be given, hidden or not!", cmd_giveforce,
                     info.usages = {"/giveforce <player: player> <item: string> [amount: int] [data: int]"};
                     info.permissions = {"primebds.command.giveforce"};
                     info.default_permission = "op";
                     info.aliases = {"givef", "forcegive"};);

    /// Forces any item registered to be given, hidden or not!
    static bool cmd_giveforce(PrimeBDS &plugin, endstone::CommandSender &sender,
                              const std::vector<std::string> &args) {
        if (args.size() < 2) {
            sender.sendMessage("\u00a7cUsage: /giveforce <player> <block> [amount] [data]");
            return false;
        }
        std::string block_id = args[1];
        std::transform(block_id.begin(), block_id.end(), block_id.begin(), ::tolower);

        int amount = 1, data = 0;
        if (args.size() >= 3) {
            try {
                amount = std::stoi(args[2]);
            } catch (...) {
                sender.sendMessage("\u00a7cInvalid amount");
                return false;
            }
            if (amount < 1 || amount > 255) {
                sender.sendMessage("\u00a7cAmount must be 1-255");
                return false;
            }
        }
        if (args.size() >= 4) {
            try {
                data = std::stoi(args[3]);
            } catch (...) {
            }
        }

        if (block_id.find("invisiblebedrock") != std::string::npos)
            block_id = "invisible_bedrock";

        auto targets = utils::getMatchingActors(plugin.getServer(), args[0], sender);
        if (targets.empty()) {
            sender.sendMessage("\u00a7cNo matching players found");
            return false;
        }

        // Create ItemStack with ItemTypeId
        auto item_type_id = endstone::ItemTypeId::minecraft(block_id);
        endstone::ItemStack item(item_type_id, amount, data);

        for (auto *t : targets) {
            auto *p = dynamic_cast<endstone::Player *>(t);
            if (p)
                p->getInventory().addItem(item);
        }

        if (targets.size() == 1) {
            auto *p = dynamic_cast<endstone::Player *>(targets[0]);
            sender.sendMessage("\u00a7e" + (p ? p->getName() : "Player") +
                               " \u00a7rwas given \u00a77x" + std::to_string(amount) +
                               " \u00a7e" + block_id);
        } else {
            sender.sendMessage("\u00a7e" + std::to_string(targets.size()) +
                               " \u00a7rplayers were given \u00a77x" + std::to_string(amount) +
                               " \u00a7e" + block_id);
        }
        return true;
    }

} // namespace primebds::commands
