/// @file alist.cpp
/// Manages server allowlist profiles and server allowlist!

#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"

#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <set>

namespace fs = std::filesystem;
using json = nlohmann::json;

namespace primebds::commands {

    static bool cmd_alist(PrimeBDS &, endstone::CommandSender &,
                        const std::vector<std::string> &);

    REGISTER_COMMAND(alist, "Manages server allowlist profiles and server allowlist!", cmd_alist,
                     info.usages = {
                         "/alist (list|check|profiles)<action: alist_action> [args: message]",
                         "/alist (add|remove)<action: alist_action> <player: string> [ignore_max_player_limit: bool]",
                         "/alist (create|use|delete|clear)<action: alist_action> <name: string>",
                         "/alist (inherit)<action: alist_action> <child: string> <parent: string>"};
                     info.permissions = {"primebds.command.alist"}; info.aliases = {"wlist"};);

    /// Gets the path to the active allowlist.json in the BDS server root
    static fs::path getAllowlistPath(PrimeBDS &plugin) {
        // getDataFolder() = {root}/plugins/{name}/
        return plugin.getDataFolder().parent_path().parent_path() / "allowlist.json";
    }

    /// Gets the allowlist profiles directory, creating it if needed
    static fs::path getProfilesDir(PrimeBDS &plugin) {
        auto dir = plugin.getDataFolder() / "allowlist_profiles";
        fs::create_directories(dir);
        return dir;
    }

    /// Reads a JSON array from a file, returning empty array on failure
    static json readJsonFile(const fs::path &path) {
        if (!fs::exists(path))
            return json::array();
        std::ifstream f(path);
        if (!f.is_open())
            return json::array();
        try {
            auto data = json::parse(f);
            return data.is_array() ? data : json::array();
        } catch (...) {
            return json::array();
        }
    }

    /// Writes a JSON value to a file with 4-space indent
    static bool writeJsonFile(const fs::path &path, const json &data) {
        fs::create_directories(path.parent_path());
        std::ofstream f(path);
        if (!f.is_open())
            return false;
        f << data.dump(4);
        return f.good();
    }

    /// Ensures the default profile exists, seeded from the live allowlist.json
    static void ensureDefaultProfile(PrimeBDS &plugin) {
        auto default_path = getProfilesDir(plugin) / "default.json";
        auto allowlist_path = getAllowlistPath(plugin);

        if (!fs::exists(default_path) && fs::exists(allowlist_path)) {
            try {
                fs::copy_file(allowlist_path, default_path);
                plugin.getLogger().info("Initialized allowlist_profiles/default.json from existing allowlist.json");
            } catch (const std::exception &e) {
                plugin.getLogger().warning("Failed to create default allowlist profile: {}", e.what());
            }
        }
    }

    /// Manages server allowlist profiles and server allowlist!
    static bool cmd_alist(PrimeBDS &plugin, endstone::CommandSender &sender,
                          const std::vector<std::string> &args) {
        if (args.empty()) {
            sender.sendMessage("\u00a7cUsage: /alist <add|remove|list|check|create|delete|use|profiles|inherit|clear> ...");
            return false;
        }

        for (auto &arg : args) {
            if (arg.find('@') != std::string::npos) {
                sender.sendMessage("\u00a7cTarget selectors are invalid for this command");
                return false;
            }
        }

        ensureDefaultProfile(plugin);

        std::string sub = args[0];
        for (auto &c : sub)
            c = static_cast<char>(std::tolower(c));

        auto &server = plugin.getServer();

        if (sub == "add") {
            if (args.size() < 2) {
                sender.sendMessage("Usage: /alist add <player> [ignore_max_player_limit]");
                return true;
            }

            std::string player_name = args[1];
            std::optional<bool> ignore_limit;

            if (args.size() >= 3) {
                std::string val = args[2];
                for (auto &c : val)
                    c = static_cast<char>(std::tolower(c));
                if (val == "true")
                    ignore_limit = true;
                else if (val == "false")
                    ignore_limit = false;
                else {
                    sender.sendMessage("\u00a7rignore_max_player_limit must be 'true' or 'false'");
                    return true;
                }
            }

            (void)server.dispatchCommand(server.getCommandSender(), "whitelist add \"" + player_name + "\"");
            sender.sendMessage("\u00a7rAdded \u00a7b" + player_name + "\u00a7r to allowlist.");

            if (ignore_limit.has_value()) {
                auto allowlist_path = getAllowlistPath(plugin);
                auto data = readJsonFile(allowlist_path);
                bool modified = false;

                for (auto &entry : data) {
                    if (entry.value("name", "") == player_name) {
                        std::string xuid;
                        auto *player = server.getPlayer(player_name);
                        if (player) {
                            xuid = player->getXuid();
                        } else {
                            auto user = plugin.db->getUserByName(player_name);
                            if (user)
                                xuid = user->xuid;
                        }

                        if (!xuid.empty()) {
                            entry["ignoresPlayerLimit"] = ignore_limit.value();
                            entry["xuid"] = xuid;
                            modified = true;
                        } else {
                            sender.sendMessage("\u00a7r" + player_name +
                                               " does not have a recorded xuid, ignore_max_player_limit could not be set");
                        }
                        break;
                    }
                }

                if (modified) {
                    writeJsonFile(allowlist_path, data);
                    plugin.getLogger().info("Updated ignoresPlayerLimit for {} to {}",
                                            player_name, ignore_limit.value());
                }
            }

            (void)server.dispatchCommand(server.getCommandSender(), "whitelist reload");
            return true;
        }

        if (sub == "remove") {
            if (args.size() < 2) {
                sender.sendMessage("Usage: /alist remove <player>");
                return true;
            }
            std::string player_name = args[1];
            (void)server.dispatchCommand(server.getCommandSender(), "whitelist remove \"" + player_name + "\"");
            sender.sendMessage("\u00a7rRemoved \u00a7b" + player_name + "\u00a7r from allowlist.");
            (void)server.dispatchCommand(server.getCommandSender(), "whitelist reload");
            return true;
        }

        if (sub == "list") {
            auto allowlist_path = getAllowlistPath(plugin);
            if (!fs::exists(allowlist_path)) {
                sender.sendMessage("\u00a7cAllowlist file not found.");
                return true;
            }

            auto data = readJsonFile(allowlist_path);
            if (data.empty()) {
                sender.sendMessage("\u00a7rAllowlist is empty.");
                return true;
            }

            std::string msg = "\u00a7rAllowlist players:";
            for (auto &entry : data) {
                std::string name = entry.value("name", "[unknown]");
                bool ignores = entry.value("ignoresPlayerLimit", false);
                if (ignores)
                    msg += "\n\u00a77- \u00a76" + name + " \u00a78(ignores player limit)";
                else
                    msg += "\n\u00a77- \u00a77" + name;
            }
            sender.sendMessage(msg);
            return true;
        }

        if (sub == "check") {
            auto profile_name = plugin.serverdb->getServerInfo().allowlist_profile;
            auto allowlist_path = getAllowlistPath(plugin);

            if (!fs::exists(allowlist_path)) {
                sender.sendMessage("\u00a7cActive allowlist.json file not found.");
                return true;
            }

            auto data = readJsonFile(allowlist_path);
            sender.sendMessage("\u00a7rUsing allowlist profile: \u00a7b" + profile_name +
                               "\u00a7r with \u00a7a" + std::to_string(data.size()) + "\u00a7r active players.");
            return true;
        }

        if (sub == "create") {
            if (args.size() < 2) {
                sender.sendMessage("Usage: /alist create <name>");
                return true;
            }

            std::string profile_name = args[1];
            auto path = getProfilesDir(plugin) / (profile_name + ".json");

            if (fs::exists(path)) {
                sender.sendMessage("Allowlist profile '" + profile_name + "' already exists.");
                return true;
            }

            if (writeJsonFile(path, json::array()))
                sender.sendMessage("Created allowlist profile '" + profile_name + "'.");
            else
                sender.sendMessage("\u00a7cFailed to create profile.");
            return true;
        }

        if (sub == "delete") {
            if (args.size() < 2) {
                sender.sendMessage("Usage: /alist delete <name>");
                return true;
            }

            std::string profile_name = args[1];
            auto path = getProfilesDir(plugin) / (profile_name + ".json");

            if (!fs::exists(path)) {
                sender.sendMessage("Allowlist profile '" + profile_name + "' does not exist.");
                return true;
            }

            try {
                fs::remove(path);
                sender.sendMessage("Deleted allowlist profile '" + profile_name + "'.");
            } catch (const std::exception &e) {
                sender.sendMessage("\u00a7cFailed to delete profile: " + std::string(e.what()));
            }
            return true;
        }

        if (sub == "use") {
            if (args.size() < 2) {
                sender.sendMessage("Usage: /alist use <profile>");
                return true;
            }

            std::string target_profile = args[1];
            auto profiles_dir = getProfilesDir(plugin);
            auto target_path = profiles_dir / (target_profile + ".json");

            if (!fs::exists(target_path)) {
                sender.sendMessage("Profile '" + target_profile + "' does not exist.");
                return true;
            }

            auto allowlist_path = getAllowlistPath(plugin);
            auto current_profile = plugin.serverdb->getServerInfo().allowlist_profile;
            auto current_profile_path = profiles_dir / (current_profile + ".json");

            // Backup current allowlist to the current profile
            if (fs::exists(allowlist_path)) {
                try {
                    fs::copy_file(allowlist_path, current_profile_path,
                                  fs::copy_options::overwrite_existing);
                    plugin.getLogger().info("Backed up allowlist.json to profile '{}'", current_profile);
                } catch (const std::exception &e) {
                    plugin.getLogger().warning("Failed to backup allowlist: {}", e.what());
                }
            }

            // Copy target profile to allowlist.json and activate
            try {
                fs::copy_file(target_path, allowlist_path, fs::copy_options::overwrite_existing);
                plugin.serverdb->updateServerInfo("allowlist_profile", target_profile);
                (void)server.dispatchCommand(server.getCommandSender(), "whitelist reload");
                sender.sendMessage("Activated allowlist profile '" + target_profile + "'");
            } catch (const std::exception &e) {
                sender.sendMessage("\u00a7cFailed to switch profile: " + std::string(e.what()));
            }
            return true;
        }

        if (sub == "profiles") {
            auto profiles_dir = getProfilesDir(plugin);
            auto current_profile = plugin.serverdb->getServerInfo().allowlist_profile;
            std::string msg;

            for (auto &entry : fs::directory_iterator(profiles_dir)) {
                if (entry.path().extension() == ".json") {
                    std::string name = entry.path().stem().string();
                    if (name == current_profile)
                        msg += "\n\u00a78- \u00a7a" + name + " \u00a77(current)";
                    else
                        msg += "\n\u00a78- \u00a77" + name;
                }
            }

            if (msg.empty()) {
                sender.sendMessage("\u00a7cNo saved allowlist profiles.");
                return true;
            }

            sender.sendMessage("Available profiles:" + msg);
            return true;
        }

        if (sub == "inherit") {
            if (args.size() < 3) {
                sender.sendMessage("Usage: /alist inherit <child> <parent>");
                return true;
            }

            std::string child_name = args[1];
            std::string parent_name = args[2];
            auto profiles_dir = getProfilesDir(plugin);
            auto child_path = profiles_dir / (child_name + ".json");
            auto parent_path = profiles_dir / (parent_name + ".json");

            if (!fs::exists(child_path)) {
                sender.sendMessage("Child profile '" + child_name + "' does not exist.");
                return true;
            }
            if (!fs::exists(parent_path)) {
                sender.sendMessage("Parent profile '" + parent_name + "' does not exist.");
                return true;
            }

            try {
                auto parent_data = readJsonFile(parent_path);
                auto child_data = readJsonFile(child_path);

                std::set<std::string> child_names;
                for (auto &e : child_data)
                    child_names.insert(e.value("name", ""));

                for (auto &e : parent_data) {
                    if (child_names.find(e.value("name", "")) == child_names.end())
                        child_data.push_back(e);
                }

                writeJsonFile(child_path, child_data);
                sender.sendMessage("Inherited " + std::to_string(parent_data.size()) +
                                   " entries from '" + parent_name + "' into '" + child_name + "'");

                auto current_profile = plugin.serverdb->getServerInfo().allowlist_profile;
                if (current_profile == child_name) {
                    writeJsonFile(getAllowlistPath(plugin), child_data);
                    (void)server.dispatchCommand(server.getCommandSender(), "whitelist reload");
                    sender.sendMessage("Updated live allowlist.json and reloaded whitelist for active profile '" +
                                       child_name + "'");
                }
            } catch (const std::exception &e) {
                sender.sendMessage("\u00a7cFailed to inherit profile: " + std::string(e.what()));
            }
            return true;
        }

        if (sub == "clear") {
            if (args.size() < 2) {
                sender.sendMessage("Usage: /alist clear <profile>");
                return true;
            }

            std::string target_profile = args[1];
            auto profiles_dir = getProfilesDir(plugin);
            auto target_path = profiles_dir / (target_profile + ".json");

            if (!fs::exists(target_path)) {
                sender.sendMessage("Profile '" + target_profile + "' does not exist.");
                return true;
            }

            auto data = readJsonFile(target_path);
            if (data.empty()) {
                sender.sendMessage("Profile '" + target_profile + "' is already empty.");
                return true;
            }

            for (auto &entry : data) {
                std::string name = entry.value("name", "");
                if (!name.empty())
                    (void)server.dispatchCommand(server.getCommandSender(), "whitelist remove \"" + name + "\"");
            }

            writeJsonFile(target_path, json::array());

            auto current_profile = plugin.serverdb->getServerInfo().allowlist_profile;
            if (current_profile == target_profile) {
                writeJsonFile(getAllowlistPath(plugin), json::array());
                (void)server.dispatchCommand(server.getCommandSender(), "whitelist reload");
                sender.sendMessage("Cleared and reloaded active profile '" + target_profile + "'");
            } else {
                sender.sendMessage("Cleared profile '" + target_profile + "'");
            }
            return true;
        }

        sender.sendMessage("\u00a7cUnknown subcommand for /alist");
        return false;
    }


} // namespace primebds::commands
