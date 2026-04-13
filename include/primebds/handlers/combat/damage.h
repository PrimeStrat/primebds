/// @file damage.h
/// Damage event handler with god mode, custom damage modifiers, and hit cooldowns.

#pragma once

#include <endstone/endstone.hpp>

namespace primebds
{
    class PrimeBDS;
}

namespace primebds::handlers::combat
{

    void handleDamageEvent(PrimeBDS &plugin, endstone::ActorDamageEvent &event);

} // namespace primebds::handlers::combat
