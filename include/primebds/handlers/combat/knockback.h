/// @file knockback.h
/// Knockback event handler with custom modifiers and sprint/projectile handling.

#pragma once

#include <endstone/endstone.hpp>

namespace primebds
{
    class PrimeBDS;
}

namespace primebds::handlers::combat
{

    void handleKnockbackEvent(PrimeBDS &plugin, endstone::ActorKnockbackEvent &event);

} // namespace primebds::handlers::combat
