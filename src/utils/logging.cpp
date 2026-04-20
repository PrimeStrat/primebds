/// @file logging.cpp
/// Logging utilities and Discord webhook relay.

#include "primebds/utils/logging.h"
#include "primebds/utils/config/config_manager.h"

#include <algorithm>
#include <iostream>
#include <regex>
#include <sstream>

#ifdef _WIN32
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")
#else
#include <cstring>
#endif

namespace primebds::utils {

    static std::string stripColorCodes(const std::string &msg) {
        static const std::regex color_re("§.");
        return std::regex_replace(msg, color_re, "");
    }

    void log(endstone::Server &server, const std::string &message,
             const std::string &type, const std::string &permission) {
        // Discord relay - called synchronously to avoid dangling threads after plugin reload
        discordRelay(message, type);

        // Notify online players with the appropriate permission
        if (permission.empty())
            return;

        for (auto *player : server.getOnlinePlayers()) {
            if (player->hasPermission(permission))
                player->sendMessage(message);
        }
    }

    void discordRelay(const std::string &message, const std::string &type) {
        auto cleaned = stripColorCodes(message);

        auto &cfg = config::ConfigManager::instance();
        auto discord = cfg.getModule("discord_webhook");

        // Determine webhook URL
        std::string webhook_url;
        if (type == "cmd" && discord.value("command_logs", nlohmann::json::object()).value("enabled", false))
            webhook_url = discord["command_logs"].value("webhook", "");
        else if (type == "mod" && discord.value("moderation_logs", nlohmann::json::object()).value("enabled", false))
            webhook_url = discord["moderation_logs"].value("webhook", "");
        else if (type == "chat" && discord.value("chat_logs", nlohmann::json::object()).value("enabled", false))
            webhook_url = discord["chat_logs"].value("webhook", "");
        else if (type == "connections" && discord.value("connection_logs", nlohmann::json::object()).value("enabled", false))
            webhook_url = discord["connection_logs"].value("webhook", "");

        if (webhook_url.empty())
            return;

        // Build payload
        nlohmann::json payload;
        auto embed_cfg = discord.value("embed_for_log", nlohmann::json::object());
        if (embed_cfg.value("enabled", false)) {
            payload["embeds"] = nlohmann::json::array({{{"title", embed_cfg.value("title", "Log")},
                                                        {"description", cleaned},
                                                        {"color", embed_cfg.value("color", 0x3498db)}}});
        } else {
            payload["content"] = cleaned;
        }

        // Note: Full HTTP POST implementation depends on the platform's HTTP library.
        // Endstone may provide HTTP utilities; for now we log to console if webhook is configured.
        std::cout << "[PrimeBDS Discord] " << type << ": " << cleaned << "\n";
    }

} // namespace primebds::utils
