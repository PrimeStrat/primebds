#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"

#include <ctime>

namespace primebds::commands
{

    static bool cmd_note(PrimeBDS &plugin, endstone::CommandSender &sender,
                         const std::vector<std::string> &args)
    {
        if (args.size() < 2)
        {
            sender.sendMessage("\u00a7cUsage: /note <player> <add/remove/clear/list> [text/id]");
            return false;
        }

        for (auto &a : args)
            if (a.find('@') != std::string::npos)
            {
                sender.sendMessage("\u00a7cTarget selectors are invalid for this command");
                return false;
            }

        std::string target_name = args[0];
        std::string sub = args[1];

        auto user = plugin.db->getUserByName(target_name);
        if (!user)
        {
            sender.sendMessage("\u00a7cPlayer not found");
            return false;
        }

        if (sub == "add")
        {
            if (args.size() < 3)
            {
                sender.sendMessage("\u00a7cProvide note text");
                return false;
            }
            std::string text;
            for (size_t i = 2; i < args.size(); ++i)
            {
                if (i > 2)
                    text += " ";
                text += args[i];
            }
            plugin.db->addNote(user->xuid, user->name, text, sender.getName());
            sender.sendMessage("\u00a7aNote added for \u00a7e" + target_name);
            return true;
        }

        if (sub == "remove")
        {
            if (args.size() < 3)
            {
                sender.sendMessage("\u00a7cProvide note ID");
                return false;
            }
            int id = std::atoi(args[2].c_str());
            plugin.db->removeNote(id);
            sender.sendMessage("\u00a7aNote " + std::to_string(id) + " removed");
            return true;
        }

        if (sub == "clear")
        {
            auto notes = plugin.db->getNotes(user->xuid);
            for (auto &n : notes)
                plugin.db->removeNote(n.id);
            sender.sendMessage("\u00a7aAll notes cleared for \u00a7e" + target_name);
            return true;
        }

        if (sub == "list")
        {
            int page = (args.size() > 2) ? std::max(1, std::atoi(args[2].c_str())) : 1;
            auto notes = plugin.db->getNotes(user->xuid);
            if (notes.empty())
            {
                sender.sendMessage("\u00a7eNo notes for " + target_name);
                return true;
            }

            int per_page = 5;
            int total_pages = ((int)notes.size() + per_page - 1) / per_page;
            page = std::min(page, total_pages);
            int start = (page - 1) * per_page;
            int end = std::min(start + per_page, (int)notes.size());

            sender.sendMessage("\u00a76Notes for \u00a7e" + target_name + " \u00a77(Page " +
                               std::to_string(page) + "/" + std::to_string(total_pages) + "):");
            for (int i = start; i < end; ++i)
            {
                sender.sendMessage("\u00a78[\u00a77" + std::to_string(notes[i].id) +
                                   "\u00a78] \u00a7f" + notes[i].note +
                                   " \u00a77- \u00a7e" + notes[i].added_by);
            }
            return true;
        }

        sender.sendMessage("\u00a7cUnknown action. Use add/remove/clear/list");
        return false;
    }

    REGISTER_COMMAND(note, "Add, remove, clear, or list notes on a player!", cmd_note,
                     info.usages = {
                         "/note <player: player> (add) <text: message>",
                         "/note <player: player> (remove) <id: int>",
                         "/note <player: player> (clear)",
                         "/note <player: player> (list) [page: int]"};
                     info.permissions = {"primebds.command.note"};);

} // namespace primebds::commands
