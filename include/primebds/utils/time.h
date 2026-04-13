/// @file time.h
/// Timezone conversion utilities.

#pragma once

#include <cstdint>
#include <string>

namespace primebds::utils
{

    bool isDst(int64_t timestamp);
    std::string convertToTimezone(int64_t timestamp, const std::string &timezone);

} // namespace primebds::utils
