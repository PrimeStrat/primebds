/// @file command_metadata.h
/// Centralized Endstone command registration — single source of truth for all
/// command usages, permissions, and aliases.

#pragma once

#include <endstone/plugin/plugin.h>

namespace primebds {

/// Registers every plugin command with the Endstone description builder.
/// Called from the ENDSTONE_PLUGIN macro body so that command metadata lives
/// in exactly one place.
void registerEndstoneCommands(endstone::detail::PluginDescriptionBuilder &builder);

} // namespace primebds
