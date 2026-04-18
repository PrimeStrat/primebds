/// @file primebds.cpp
/// PrimeBDS management command!

#include "primebds/commands/command_registry.h"
#include "primebds/plugin.h"
#include "primebds/utils/config/config_manager.h"
#include "primebds/utils/forms.h"

#include <algorithm>
#include <sstream>
#include <string>
#include <vector>

namespace primebds::commands {

    static bool cmd_primebds(PrimeBDS &, endstone::CommandSender &,
                        const std::vector<std::string> &);

    REGISTER_COMMAND(primebds, "PrimeBDS management command!", cmd_primebds,
                     info.usages = {
                         "/primebds",
                         "/primebds (info|reloadconfig)<action: pbds_action> [args: message]",
                         "/primebds (config)<action: pbds_action> <keypath: string> [value: message]"};
                     info.permissions = {"primebds.command.primebds"};);

    namespace {
        nlohmann::json *resolveJsonPath(nlohmann::json &root, const std::string &path) {
            nlohmann::json *node = &root;
            std::istringstream stream(path);
            std::string segment;
            while (std::getline(stream, segment, '.')) {
                if (!node->is_object() || !node->contains(segment))
                    return nullptr;
                node = &(*node)[segment];
            }
            return node;
        }

        void setJsonPath(nlohmann::json &root, const std::string &path, const nlohmann::json &value) {
            nlohmann::json *node = &root;
            std::vector<std::string> segments;
            std::istringstream stream(path);
            std::string segment;
            while (std::getline(stream, segment, '.'))
                segments.push_back(segment);

            for (size_t i = 0; i < segments.size() - 1; ++i) {
                if (!node->contains(segments[i]) || !(*node)[segments[i]].is_object())
                    (*node)[segments[i]] = nlohmann::json::object();
                node = &(*node)[segments[i]];
            }
            (*node)[segments.back()] = value;
        }

        std::string formatLabel(const std::string &key) {
            std::string out;
            bool cap = true;
            for (char c : key) {
                if (c == '_') {
                    out += ' ';
                    cap = true;
                } else {
                    out += cap ? static_cast<char>(std::toupper(c)) : c;
                    cap = false;
                }
            }
            return out;
        }

        // ---------------------------------------------------------------
        // Form-based config editor
        // ---------------------------------------------------------------

        // Forward declarations
        void openConfigCategories(PrimeBDS &plugin, endstone::Player &player);

        /// Apply form submissions to a JSON section and save config.
        void applyFormValues(const std::vector<std::string> &keys, nlohmann::json &section,
                             const std::vector<std::variant<int, float, std::string, bool>> &values) {
            for (size_t i = 0; i < keys.size() && i < values.size(); ++i) {
                auto &val = section[keys[i]];
                auto &submitted = values[i];
                if (val.is_boolean()) {
                    if (auto *b = std::get_if<bool>(&submitted))
                        val = *b;
                } else if (val.is_number_integer()) {
                    if (auto *s = std::get_if<std::string>(&submitted)) {
                        try { val = std::stoi(*s); } catch (...) {}
                    } else if (auto *iv = std::get_if<int>(&submitted))
                        val = *iv;
                } else if (val.is_number_float()) {
                    if (auto *s = std::get_if<std::string>(&submitted)) {
                        try { val = std::stod(*s); } catch (...) {}
                    } else if (auto *fv = std::get_if<float>(&submitted))
                        val = *fv;
                } else if (val.is_string()) {
                    if (auto *s = std::get_if<std::string>(&submitted))
                        val = *s;
                } else if (val.is_array()) {
                    if (auto *s = std::get_if<std::string>(&submitted)) {
                        try { val = nlohmann::json::parse(*s); } catch (...) {}
                    }
                }
            }
        }

        /// Open a ModalForm to edit primitive values at modules.<module_key>.<sub_key>.
        void openSubSectionEditor(PrimeBDS &plugin, endstone::Player &player,
                                  const std::string &title, const std::string &module_key,
                                  const std::string &sub_key) {
            auto &cfg = config::ConfigManager::instance();
            auto &conf = cfg.config();
            if (!conf.contains("modules") || !conf["modules"].contains(module_key) ||
                !conf["modules"][module_key].contains(sub_key))
                return;
            auto &section = conf["modules"][module_key][sub_key];

            std::vector<std::string> keys;
            for (auto &[k, v] : section.items()) {
                if (!v.is_object())
                    keys.push_back(k);
            }
            if (keys.empty()) {
                player.sendMessage("\u00a7cNo editable values in " + title);
                return;
            }

            utils::ModalFormBuilder form;
            form.title(title);
            for (auto &key : keys) {
                auto &val = section[key];
                if (val.is_boolean())
                    form.toggle(formatLabel(key), val.get<bool>());
                else
                    form.textInput(formatLabel(key), val.dump(), val.dump());
            }

            auto player_name = player.getName();
            form.show(player, [keys, module_key, sub_key, player_name, &plugin](auto result) {
                if (!result.has_value())
                    return;
                auto &cfg = config::ConfigManager::instance();
                auto &section = cfg.config()["modules"][module_key][sub_key];
                applyFormValues(keys, section, result.value());
                cfg.save();
                auto *p = plugin.getServer().getPlayer(player_name);
                if (p)
                    p->sendMessage("\u00a7aConfig saved!");
            });
        }

        /// Open a ModalForm to edit primitive values at modules.<module_key>.
        void openPrimitiveEditor(PrimeBDS &plugin, endstone::Player &player,
                                 const std::string &title, const std::string &module_key) {
            auto &cfg = config::ConfigManager::instance();
            auto &conf = cfg.config();
            if (!conf.contains("modules") || !conf["modules"].contains(module_key))
                return;
            auto &section = conf["modules"][module_key];

            std::vector<std::string> keys;
            for (auto &[k, v] : section.items()) {
                if (!v.is_object())
                    keys.push_back(k);
            }
            if (keys.empty()) {
                player.sendMessage("\u00a7cNo editable values in " + title);
                return;
            }

            utils::ModalFormBuilder form;
            form.title(title + " Configuration");
            for (auto &key : keys) {
                auto &val = section[key];
                if (val.is_boolean())
                    form.toggle(formatLabel(key), val.get<bool>());
                else
                    form.textInput(formatLabel(key), val.dump(), val.dump());
            }

            auto player_name = player.getName();
            form.show(player, [keys, module_key, player_name, &plugin](auto result) {
                if (!result.has_value())
                    return;
                auto &cfg = config::ConfigManager::instance();
                auto &section = cfg.config()["modules"][module_key];
                applyFormValues(keys, section, result.value());
                cfg.save();
                auto *p = plugin.getServer().getPlayer(player_name);
                if (p)
                    p->sendMessage("\u00a7aConfig saved!");
            });
        }

        /// Open an ActionForm showing sub-categories within a module.
        void openModuleEditor(PrimeBDS &plugin, endstone::Player &player,
                              const std::string &module_name) {
            auto &cfg = config::ConfigManager::instance();
            auto &conf = cfg.config();
            if (!conf.contains("modules") || !conf["modules"].contains(module_name))
                return;
            auto &section = conf["modules"][module_name];

            std::vector<std::string> sub_keys;
            bool has_primitives = false;
            for (auto &[k, v] : section.items()) {
                if (v.is_object())
                    sub_keys.push_back(k);
                else
                    has_primitives = true;
            }

            if (sub_keys.empty()) {
                openPrimitiveEditor(plugin, player, formatLabel(module_name), module_name);
                return;
            }

            utils::ActionFormBuilder form;
            form.title(formatLabel(module_name));
            form.body("Select a subcategory to edit:");
            if (has_primitives)
                form.button("\u00a74Settings");
            for (auto &sub : sub_keys)
                form.button("\u00a74" + formatLabel(sub));

            auto player_name = player.getName();
            form.show(player, [&plugin, module_name, sub_keys, has_primitives, player_name](auto selection) {
                if (!selection.has_value())
                    return;
                auto *p = plugin.getServer().getPlayer(player_name);
                if (!p)
                    return;

                int idx = selection.value();
                if (has_primitives && idx == 0) {
                    openPrimitiveEditor(plugin, *p, formatLabel(module_name), module_name);
                    return;
                }
                int sub_idx = has_primitives ? idx - 1 : idx;
                if (sub_idx < 0 || sub_idx >= static_cast<int>(sub_keys.size()))
                    return;
                openSubSectionEditor(plugin, *p,
                                     formatLabel(module_name) + " > " + formatLabel(sub_keys[sub_idx]),
                                     module_name, sub_keys[sub_idx]);
            });
        }

        /// Open a ModalForm to toggle commands on/off.
        void openCommandEditor(PrimeBDS &plugin, endstone::Player &player) {
            auto &cfg = config::ConfigManager::instance();
            auto cmd_config = cfg.loadCommandConfig();

            std::vector<std::string> cmd_names;
            for (auto &[name, _] : cmd_config.items())
                cmd_names.push_back(name);
            std::sort(cmd_names.begin(), cmd_names.end());

            utils::ModalFormBuilder form;
            form.title("Commands Configuration");
            for (auto &cmd : cmd_names) {
                bool enabled = cmd_config[cmd].value("enabled", true);
                form.toggle("/" + cmd, enabled);
            }

            auto player_name = player.getName();
            form.show(player, [cmd_names, player_name, &plugin](auto result) {
                if (!result.has_value())
                    return;
                auto &values = result.value();
                auto &cfg = config::ConfigManager::instance();
                auto cmd_config = cfg.loadCommandConfig();
                for (size_t i = 0; i < cmd_names.size() && i < values.size(); ++i) {
                    if (auto *b = std::get_if<bool>(&values[i]))
                        cmd_config[cmd_names[i]]["enabled"] = *b;
                }
                cfg.saveCommandConfig(cmd_config);
                auto *p = plugin.getServer().getPlayer(player_name);
                if (p)
                    p->sendMessage("\u00a7aCommand config saved!");
            });
        }

        /// Open the top-level config categories form.
        void openConfigCategories(PrimeBDS &plugin, endstone::Player &player) {
            auto &cfg = config::ConfigManager::instance();
            auto &conf = cfg.config();

            utils::ActionFormBuilder form;
            form.title("PrimeBDS Config");
            form.body("Select a category to edit:");

            std::vector<std::string> categories;
            if (conf.contains("modules") && conf["modules"].is_object()) {
                for (auto &[k, v] : conf["modules"].items())
                    categories.push_back(k);
            }
            std::sort(categories.begin(), categories.end());
            categories.push_back("commands");

            for (auto &cat : categories)
                form.button("\u00a74" + formatLabel(cat));
            form.button("Close");

            auto player_name = player.getName();
            form.show(player, [&plugin, categories, player_name](auto selection) {
                if (!selection.has_value())
                    return;
                auto *p = plugin.getServer().getPlayer(player_name);
                if (!p)
                    return;

                int idx = selection.value();
                if (idx < 0 || idx >= static_cast<int>(categories.size()))
                    return;

                std::string chosen = categories[idx];
                if (chosen == "commands")
                    openCommandEditor(plugin, *p);
                else
                    openModuleEditor(plugin, *p, chosen);
            });
        }

    } // anonymous namespace

    /// PrimeBDS management command handler.
    static bool cmd_primebds(PrimeBDS &plugin, endstone::CommandSender &sender,
                             const std::vector<std::string> &args) {
        auto *player = sender.asPlayer();

        if (args.empty()) {
            if (player && player->isOp()) {
                openConfigCategories(plugin, *player);
            } else {
                sender.sendMessage("\u00a7dPrimeBDS v3.4.1");
                sender.sendMessage("\u00a77Use /primebds <config|reloadconfig|info> for more options");
            }
            return true;
        }

        std::string sub = args[0];
        for (auto &c : sub)
            c = (char)std::tolower(c);

        if (sub == "info") {
            sender.sendMessage("\u00a7dPrimeBDS");
            sender.sendMessage("\u00a7dAn all-in-one essentials plugin for Minecraft Bedrock Edition.");
            sender.sendMessage("");
            sender.sendMessage("\u00a7dIf this plugin has helped you, consider leaving a star:");
            sender.sendMessage("\u00a7e@ https://github.com/PrimeStrat/primebds");
            sender.sendMessage("");
            sender.sendMessage("\u00a7dConfused on how something works?");
            sender.sendMessage("\u00a7dVisit the wiki:");
            sender.sendMessage("\u00a7e@ https://github.com/PrimeStrat/primebds/wiki");
            return true;
        }

        if (sub == "reloadconfig") {
            config::ConfigManager::instance().reload();
            sender.sendMessage("\u00a7dPrimeBDS config reloaded!");
            return true;
        }

        if (sub == "config") {
            if (!player) {
                sender.sendMessage("\u00a7cThis subcommand can only be used by a player");
                return false;
            }
            if (!player->isOp()) {
                sender.sendMessage("\u00a7cOnly operators can modify the server config");
                return false;
            }

            if (args.size() < 2) {
                openConfigCategories(plugin, *player);
                return true;
            }

            std::string key = args[1];

            if (args.size() == 2) {
                auto &cfg = config::ConfigManager::instance();
                auto &conf = cfg.config();
                auto *val = resolveJsonPath(conf, key);
                if (!val)
                    sender.sendMessage("\u00a7cConfig key \u00a7e" + key + " \u00a7cnot found");
                else
                    sender.sendMessage("\u00a7e" + key + " \u00a7a= \u00a7e" + val->dump());
                return true;
            }

            std::string value;
            for (size_t i = 2; i < args.size(); ++i) {
                if (i > 2)
                    value += " ";
                value += args[i];
            }

            nlohmann::json jval;
            try {
                jval = nlohmann::json::parse(value);
            } catch (...) {
                jval = value;
            }

            auto &cfg = config::ConfigManager::instance();
            setJsonPath(cfg.config(), key, jval);
            cfg.save();
            sender.sendMessage("\u00a7e" + key + " \u00a7aset to \u00a7e" + jval.dump());
            return true;
        }

        sender.sendMessage("\u00a7cUnknown subcommand. Use /primebds [config|reloadconfig|info]");
        return false;
    }

} // namespace primebds::commands
