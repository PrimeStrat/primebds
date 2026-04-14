/// @file crasher_patch.h
/// Crasher patch: blocks /me command with excessive @e selectors (DoS protection).

#pragma once

#include <endstone/endstone.hpp>
#include <string>

namespace primebds {
    class PrimeBDS;
}

namespace primebds::handlers::preprocesses {

    bool checkCrasherExploit(PrimeBDS &plugin, endstone::Player &player, const std::string &command);

} // namespace primebds::handlers::preprocesses
