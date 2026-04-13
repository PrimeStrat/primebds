/// @file command_intercept.h
/// Command preprocessing: moderation command remapping, exemption checks.

#pragma once

#include <endstone/endstone.hpp>

namespace primebds
{
    class PrimeBDS;
}

namespace primebds::handlers::preprocesses
{

    void handleCommandPreprocess(PrimeBDS &plugin, endstone::PlayerCommandEvent &event);
    void handleServerCommandPreprocess(PrimeBDS &plugin, endstone::ServerCommandEvent &event);

} // namespace primebds::handlers::preprocesses
