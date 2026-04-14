/// @file bossbar.cpp
/// Sets or clears a client-sided bossbar display!

#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"
#include "primebds/utils/target_selector.h"

#include <algorithm>
#include <map>
#include <memory>
#include <string>

namespace primebds::commands {

    static bool cmd_bossbar(PrimeBDS &, endstone::CommandSender &,
                        const std::vector<std::string> &);

    REGISTER_COMMAND(bossbar, "Sets or clears a client-sided bossbar display!", cmd_bossbar,
                     info.usages = {
                         "/bossbar <player: player> (red|blue|green|yellow|pink|purple|rebecca_purple|white) <percent: float> <title: message>",
                         "/bossbar <player: player> (clear)"};
                     info.permissions = {"primebds.command.bossbar"};);

    // Cache of active bossbars per player (by name)
    static std::map<std::string, std::unique_ptr<endstone::BossBar>> boss_bar_cache;

    static endstone::BarColor parse_bar_color(const std::string &color_str) {
        std::string lower = color_str;
        std::transform(lower.begin(), lower.end(), lower.begin(),
                       [](unsigned char c)
                       { return std::tolower(c); });

        if (lower == "pink")
            return endstone::BarColor::Pink;
        if (lower == "blue")
            return endstone::BarColor::Blue;
        if (lower == "red")
            return endstone::BarColor::Red;
        if (lower == "green")
            return endstone::BarColor::Green;
        if (lower == "yellow")
            return endstone::BarColor::Yellow;
        if (lower == "purple")
            return endstone::BarColor::Purple;
        if (lower == "rebecca_purple" || lower == "rebeccapurple")
            return endstone::BarColor::RebeccaPurple;
        if (lower == "white")
            return endstone::BarColor::White;
        return endstone::BarColor::Red; // default
    }

    /// Sets or clears a client-sided bossbar display!
    static bool cmd_bossbar(PrimeBDS &plugin, endstone::CommandSender &sender,
                            const std::vector<std::string> &args) {
        if (args.size() < 2) {
            sender.sendMessage("\u00a7cUsage: /bossbar <player> <color> <percent> <title> OR /bossbar <player> clear");
            return false;
        }

        std::string target_selector = args[0];

        // Handle clear subcommand
        if (args.size() == 2) {
            std::string sub = args[1];
            std::transform(sub.begin(), sub.end(), sub.begin(),
                           [](unsigned char c)
                           { return std::tolower(c); });

            if (sub == "clear") {
                auto targets = utils::getMatchingActors(plugin.getServer(), target_selector, sender);
                int removed = 0;
                for (auto *t : targets) {
                    if (auto *p = dynamic_cast<endstone::Player *>(t)) {
                        auto it = boss_bar_cache.find(p->getName());
                        if (it != boss_bar_cache.end()) {
                            it->second->removePlayer(*p);
                            boss_bar_cache.erase(it);
                            removed++;
                        }
                    }
                }
                sender.sendMessage("\u00a7aRemoved boss bar from " + std::to_string(removed) + " player(s)");
                return true;
            }
        }

        // Set bossbar: /bossbar <player> <color> <percent> <title...>
        if (args.size() < 4) {
            sender.sendMessage("\u00a7cUsage: /bossbar <player> <color> <percent> <title>");
            return false;
        }

        endstone::BarColor color = parse_bar_color(args[1]);

        float percent;
        try {
            percent = std::stof(args[2]);
            if (percent < 0.0f)
                percent = 0.0f;
            if (percent > 100.0f)
                percent = 100.0f;
        } catch (...) {
            sender.sendMessage("\u00a7cInvalid percent value '" + args[2] + "'. Must be a number 0-100.");
            return false;
        }

        // Join remaining args as title
        std::string title;
        for (size_t i = 3; i < args.size(); i++) {
            if (i > 3)
                title += " ";
            title += args[i];
        }

        auto targets = utils::getMatchingActors(plugin.getServer(), target_selector, sender);
        if (targets.empty()) {
            sender.sendMessage("\u00a7cNo matching players found for " + target_selector + "!");
            return false;
        }

        for (auto *t : targets) {
            auto *p = dynamic_cast<endstone::Player *>(t);
            if (!p)
                continue;

            // Remove existing bossbar if present
            auto it = boss_bar_cache.find(p->getName());
            if (it != boss_bar_cache.end()) {
                it->second->removePlayer(*p);
                boss_bar_cache.erase(it);
            }

            // Create new bossbar
            auto bar = plugin.getServer().createBossBar(title, color, endstone::BarStyle::Solid);
            bar->setProgress(percent / 100.0f);
            bar->addPlayer(*p);
            boss_bar_cache[p->getName()] = std::move(bar);
        }

        sender.sendMessage(
            "\u00a7bBossbar Set For " + std::to_string(targets.size()) + " player(s):\n"
                                                                         "\u00a77- \u00a7eColor: \u00a7r" +
            args[1] + "\n"
                      "\u00a77- \u00a7ePercent: \u00a7r" +
            args[2] + "%\n"
                      "\u00a77- \u00a7eTitle: \u00a7r" +
            title);
        return true;
    }


} // namespace primebds::commands
