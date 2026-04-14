/// @file reply.cpp
/// Reply to the last person who messaged you!

#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"

namespace primebds::commands {

    static bool cmd_reply(PrimeBDS &, endstone::CommandSender &,
                        const std::vector<std::string> &);

    REGISTER_COMMAND(reply, "Reply to the last person who messaged you!", cmd_reply,
                     info.usages = {"/reply <message: message>"};
                     info.permissions = {"primebds.command.reply"};
                     info.aliases = {"r"};);

    /// Reply to the last person who messaged you!
    static bool cmd_reply(PrimeBDS &plugin, endstone::CommandSender &sender,
                          const std::vector<std::string> &args) {
        auto *player = sender.asPlayer();
        if (!player) {
            sender.sendMessage("\u00a7cOnly players can use this command.");
            return true;
        }

        if (args.empty()) {
            sender.sendMessage("\u00a7cUsage: /reply <message>");
            return false;
        }

        auto user = plugin.db->getOnlineUser(player->getXuid());
        if (!user || user->last_messaged.empty()) {
            sender.sendMessage("\u00a7cYou have nobody to reply to");
            return true;
        }

        auto *target = plugin.getServer().getPlayer(user->last_messaged);
        if (!target) {
            sender.sendMessage("\u00a7c" + user->last_messaged + " is no longer online");
            return true;
        }

        std::string msg;
        for (size_t i = 0; i < args.size(); ++i) {
            if (i > 0)
                msg += " ";
            msg += args[i];
        }

        player->sendMessage("\u00a77[\u00a7eme \u00a77-> \u00a7e" + target->getName() + "\u00a77] \u00a7f" + msg);
        target->sendMessage("\u00a77[\u00a7e" + player->getName() + " \u00a77-> \u00a7eme\u00a77] \u00a7f" + msg);

        // Update last_messaged for both sides
        plugin.db->updateUser(target->getXuid(), "last_messaged", player->getName());

        // Social spy relay
        for (auto *p : plugin.getServer().getOnlinePlayers()) {
            if (p == player || p == target)
                continue;
            auto u = plugin.db->getOnlineUser(p->getXuid());
            if (u && u->enabled_ss) {
                p->sendMessage("\u00a78[\u00a7eSocialSpy\u00a78] \u00a77" + player->getName() +
                               " -> " + target->getName() + ": \u00a7f" + msg);
            }
        }
        return true;
    }

} // namespace primebds::commands
