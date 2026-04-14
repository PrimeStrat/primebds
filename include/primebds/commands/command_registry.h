/// @file command_registry.h
/// Command registration and dispatch infrastructure.

#pragma once

#include <endstone/endstone.hpp>
#include <functional>
#include <map>
#include <string>
#include <vector>

namespace primebds {

    class PrimeBDS;

    using CommandHandler = std::function<bool(PrimeBDS &, endstone::CommandSender &,
                                              const std::vector<std::string> &)>;

    struct CommandInfo {
        std::string name;
        std::string description;
        std::vector<std::string> usages;
        std::vector<std::string> permissions;
        std::string default_permission;
        std::vector<std::string> aliases;
    };

    struct CommandRegistration {
        CommandInfo info;
        CommandHandler handler;
    };

    class CommandRegistry {
    public:
        static CommandRegistry &instance();

        void registerCommand(const CommandInfo &info, CommandHandler handler);

        const std::map<std::string, CommandRegistration> &commands() const { return commands_; }
        const CommandRegistration *find(const std::string &name) const;

    private:
        CommandRegistry() = default;
        std::map<std::string, CommandRegistration> commands_;
    };

// Macro for auto-registering commands at static init time
#define REGISTER_COMMAND(cmd_name, desc, handler_func, ...) \
    static bool cmd_name##_registered = []() {                \
        CommandInfo info;                                      \
        info.name = #cmd_name;                                \
        info.description = desc;                              \
        __VA_ARGS__;                                          \
        CommandRegistry::instance().registerCommand(info, handler_func); \
        return true; }()

} // namespace primebds
