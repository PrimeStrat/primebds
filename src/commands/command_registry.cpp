/// @file command_registry.cpp
/// Command registration and dispatch infrastructure.

#include "primebds/commands/command_registry.h"

namespace primebds
{

    CommandRegistry &CommandRegistry::instance()
    {
        static CommandRegistry inst;
        return inst;
    }

    void CommandRegistry::registerCommand(const CommandInfo &info, CommandHandler handler)
    {
        CommandRegistration reg{info, std::move(handler)};
        commands_[info.name] = std::move(reg);

        // Also register aliases
        for (auto &alias : info.aliases)
        {
            commands_[alias] = commands_[info.name];
        }
    }

    const CommandRegistration *CommandRegistry::find(const std::string &name) const
    {
        auto it = commands_.find(name);
        if (it != commands_.end())
            return &it->second;
        return nullptr;
    }

} // namespace primebds
