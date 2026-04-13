/// @file whisper.h
/// Whisper/private message enhancements and social spy relay.

#pragma once

#include <endstone/endstone.hpp>
#include <string>

namespace primebds
{
    class PrimeBDS;
}

namespace primebds::handlers::preprocesses
{

    bool handleWhisperCommand(PrimeBDS &plugin, endstone::Player &sender,
                              const std::string &target_name, const std::string &message);

} // namespace primebds::handlers::preprocesses
