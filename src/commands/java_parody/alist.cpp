#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"

namespace primebds::commands
{

    static bool cmd_alist(PrimeBDS &plugin, endstone::CommandSender &sender,
                          const std::vector<std::string> &args)
    {
        if (args.empty())
        {
            sender.sendMessage("\u00a7cUsage: /alist <add|remove|list|check|create|delete|use|profiles|inherit|clear> ...");
            return false;
        }

        std::string sub = args[0];
        for (auto &c : sub)
            c = (char)std::tolower(c);

        if (sub == "list")
        {
            auto allowed = plugin.serverdb->getAllowlistEntries();
            if (allowed.empty())
            {
                sender.sendMessage("\u00a7cAllowlist is empty");
                return true;
            }
            sender.sendMessage("\u00a7a--- Allowlist ---");
            for (auto &name : allowed)
                sender.sendMessage("\u00a77- \u00a7e" + name);
            return true;
        }

        if (sub == "add" && args.size() >= 2)
        {
            std::string name = args[1];
            if (plugin.serverdb->addToAllowlist(name))
            {
                sender.sendMessage("\u00a7e" + name + " \u00a7aadded to allowlist");
            }
            else
            {
                sender.sendMessage("\u00a7e" + name + " \u00a7cis already on the allowlist");
            }
            return true;
        }

        if (sub == "remove" && args.size() >= 2)
        {
            std::string name = args[1];
            if (plugin.serverdb->removeFromAllowlist(name))
            {
                sender.sendMessage("\u00a7e" + name + " \u00a7aremoved from allowlist");
            }
            else
            {
                sender.sendMessage("\u00a7e" + name + " \u00a7cwas not on the allowlist");
            }
            return true;
        }

        if (sub == "check" && args.size() >= 2)
        {
            std::string name = args[1];
            bool on_list = plugin.serverdb->isOnAllowlist(name);
            sender.sendMessage("\u00a7e" + name + " \u00a7ais " + (on_list ? "\u00a7aon" : "\u00a7cnot on") + " \u00a7rthe allowlist");
            return true;
        }

        if (sub == "profiles")
        {
            auto profiles = plugin.serverdb->getAllowlistProfiles();
            if (profiles.empty())
            {
                sender.sendMessage("\u00a7cNo profiles exist");
                return true;
            }
            sender.sendMessage("\u00a7a--- Allowlist Profiles ---");
            for (auto &p : profiles)
            {
                sender.sendMessage("\u00a77- \u00a7e" + p.name + (p.active ? " \u00a7a(active)" : ""));
            }
            return true;
        }

        if (sub == "create" && args.size() >= 2)
        {
            std::string profile_name = args[1];
            if (plugin.serverdb->createAllowlistProfile(profile_name))
            {
                sender.sendMessage("\u00a7aProfile \u00a7e" + profile_name + " \u00a7acreated");
            }
            else
            {
                sender.sendMessage("\u00a7cProfile \u00a7e" + profile_name + " \u00a7calready exists");
            }
            return true;
        }

        if (sub == "delete" && args.size() >= 2)
        {
            std::string profile_name = args[1];
            if (plugin.serverdb->deleteAllowlistProfile(profile_name))
            {
                sender.sendMessage("\u00a7aProfile \u00a7e" + profile_name + " \u00a7adeleted");
            }
            else
            {
                sender.sendMessage("\u00a7cProfile \u00a7e" + profile_name + " \u00a7cdoes not exist");
            }
            return true;
        }

        if (sub == "use" && args.size() >= 2)
        {
            std::string profile_name = args[1];
            if (plugin.serverdb->setActiveAllowlistProfile(profile_name))
            {
                sender.sendMessage("\u00a7aNow using profile \u00a7e" + profile_name);
            }
            else
            {
                sender.sendMessage("\u00a7cProfile \u00a7e" + profile_name + " \u00a7cdoes not exist");
            }
            return true;
        }

        if (sub == "inherit" && args.size() >= 3)
        {
            std::string profile = args[1];
            std::string parent = args[2];
            if (plugin.serverdb->setAllowlistInheritance(profile, parent))
            {
                sender.sendMessage("\u00a7aProfile \u00a7e" + profile + " \u00a7anow inherits from \u00a7e" + parent);
            }
            else
            {
                sender.sendMessage("\u00a7cFailed to set inheritance");
            }
            return true;
        }

        if (sub == "clear")
        {
            plugin.serverdb->clearAllowlist();
            sender.sendMessage("\u00a7aAllowlist cleared");
            return true;
        }

        sender.sendMessage("\u00a7cUnknown subcommand for /alist");
        return false;
    }

    REGISTER_COMMAND(alist, "Manage the allowlist profiles!", cmd_alist,
                     info.usages = {
                         "/alist (list)",
                         "/alist (add) <player: string>",
                         "/alist (remove) <player: string>",
                         "/alist (check) <player: string>",
                         "/alist (profiles)",
                         "/alist (create) <profile: string>",
                         "/alist (delete) <profile: string>",
                         "/alist (use) <profile: string>",
                         "/alist (inherit) <profile: string> <parent: string>",
                         "/alist (clear)"};
                     info.permissions = {"primebds.command.alist"};);

} // namespace primebds::commands
