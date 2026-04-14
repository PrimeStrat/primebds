/// @file moderation.h
/// Moderation utility functions for ban messages and time formatting.

#pragma once

#include <cstdint>
#include <string>

namespace primebds::utils {

    std::string formatBanMessage(const std::string &reason, int64_t expiration,
                                 const std::string &server_name = "");
    std::string formatTimeRemaining(int64_t expiration);
    int64_t safeDuration(int64_t seconds);

} // namespace primebds::utils
