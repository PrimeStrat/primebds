#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"
#include "primebds/utils/target_selector.h"
#include "primebds/utils/address.h"

namespace primebds::commands
{

    static bool cmd_send(PrimeBDS &plugin, endstone::CommandSender &sender,
                         const std::vector<std::string> &args)
    {
        if (args.size() < 2)
        {
            sender.sendMessage("\u00a7cUsage: /send <player: player> <ip:port>");
            return false;
        }

        auto targets = utils::getMatchingActors(plugin.getServer(), args[0], sender);
        std::string address = args[1];

        // Parse ip:port from address
        auto colon = address.find(':');
        if (colon == std::string::npos)
        {
            sender.sendMessage("\u00a7cInvalid address format. Use ip:port");
            return false;
        }

        std::string ip = address.substr(0, colon);
        int port = std::atoi(address.substr(colon + 1).c_str());

        if (ip.empty() || port <= 0)
        {
            sender.sendMessage("\u00a7cInvalid address format. Use ip:port");
            return false;
        }

        int count = 0;
        for (auto *t : targets)
        {
            if (auto *p = dynamic_cast<endstone::Player *>(t))
            {
                p->transfer(ip, port);
                ++count;
            }
        }

        sender.sendMessage("\u00a7aSent \u00a7e" + std::to_string(count) + " \u00a7aplayer(s) to \u00a7e" + address);
        return true;
    }

    REGISTER_COMMAND(send, "Send players to another server!", cmd_send,
                     info.usages = {"/send <player: player> <ip:port: message>"};
                     info.permissions = {"primebds.command.send"};);
} // namespace primebds::commands
