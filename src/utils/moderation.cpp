/// @file moderation.cpp
/// Moderation utility functions.

#include "primebds/utils/moderation.h"

#include <algorithm>
#include <ctime>
#include <sstream>
#include <vector>

namespace primebds::utils
{

    std::string formatBanMessage(const std::string &reason, int64_t expiration,
                                 const std::string &server_name)
    {
        std::string exp_str = formatTimeRemaining(expiration);
        std::string name = server_name.empty() ? "this server" : server_name;

        return "§cYou were banned from §6" + name + "\n"
                                                    "§cPunishment Expires: §6" +
               exp_str + "\n"
                         "§cPunishment Reason: §6" +
               reason;
    }

    std::string formatTimeRemaining(int64_t expiration)
    {
        if (expiration == 0)
            return "Never - Permanent Ban";

        auto now = std::time(nullptr);
        int64_t remaining = expiration - static_cast<int64_t>(now);

        if (remaining <= 0)
            return "Expired";

        // If over 50 years, treat as permanent
        if (remaining > 50LL * 365 * 24 * 3600)
            return "Never - Permanent Ban";

        struct TimeUnit
        {
            const char *name;
            int64_t seconds;
        };

        static const TimeUnit units[] = {
            {"year", 365 * 24 * 3600},
            {"month", 30 * 24 * 3600},
            {"week", 7 * 24 * 3600},
            {"day", 24 * 3600},
            {"hour", 3600},
            {"minute", 60},
            {"second", 1}};

        std::vector<std::string> parts;
        for (auto &u : units)
        {
            int64_t count = remaining / u.seconds;
            if (count > 0)
            {
                remaining %= u.seconds;
                std::string part = std::to_string(count) + " " + u.name;
                if (count > 1)
                    part += "s";
                parts.push_back(part);
            }
        }

        if (parts.empty())
            return "0 seconds";

        std::ostringstream ss;
        for (size_t i = 0; i < parts.size(); ++i)
        {
            if (i > 0)
                ss << ", ";
            ss << parts[i];
        }
        return ss.str();
    }

    int64_t safeDuration(int64_t seconds)
    {
        constexpr int64_t MAX_DURATION = 10LL * 365 * 24 * 3600; // 10 years
        if (seconds < 0)
            return 0;
        if (seconds > MAX_DURATION)
            return MAX_DURATION;
        return seconds;
    }

} // namespace primebds::utils
