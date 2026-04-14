/// @file whisper.cpp
/// Whisper/private message enhancements and social spy relay.

#include "primebds/handlers/preprocesses/whisper.h"
#include "primebds/plugin.h"
#include "primebds/utils/config/config_manager.h"

#include <string>

namespace primebds::handlers::preprocesses {

    bool handleWhisperCommand(PrimeBDS &plugin, endstone::Player &sender,
                              const std::string &target_name, const std::string &message) {
        auto *target = plugin.getServer().getPlayer(target_name);
        if (!target) {
            sender.sendMessage("\u00a7cPlayer '" + target_name + "' is not online");
            return true;
        }

        auto &cfg = config::ConfigManager::instance();
        auto conf = cfg.config();
        std::string whisper_prefix = conf["modules"].value(
            "/server_messages/whisper_prefix"_json_pointer, std::string("\u00a77[Whisper] "));

        sender.sendMessage(whisper_prefix + "\u00a77To " + target->getName() + ": \u00a7o" + message);
        target->sendMessage(whisper_prefix + "\u00a77From " + sender.getName() + ": \u00a7o" + message);

        return true;
    }

} // namespace primebds::handlers::preprocesses
