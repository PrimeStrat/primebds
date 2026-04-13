/// @file time.cpp
/// Timezone conversion utilities.

#include "primebds/utils/time.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

#ifdef _WIN32
// Windows doesn't have timegm; use _mkgmtime instead
inline time_t timegm(struct tm *tm) { return _mkgmtime(tm); }
#endif

namespace primebds::utils
{

    bool isDst(int64_t timestamp)
    {
        time_t t = static_cast<time_t>(timestamp);
        struct tm utc_tm;
#ifdef _WIN32
        gmtime_s(&utc_tm, &t);
#else
        gmtime_r(&t, &utc_tm);
#endif

        int year = utc_tm.tm_year + 1900;

        // DST start: Second Sunday of March at 2 AM EST (7 AM UTC)
        struct tm dst_start = {};
        dst_start.tm_year = year - 1900;
        dst_start.tm_mon = 2; // March
        dst_start.tm_mday = 1;
        dst_start.tm_hour = 7; // 2 AM EST = 7 AM UTC

        // Find second Sunday of March
        time_t ds_time = timegm(&dst_start);
#ifdef _WIN32
        gmtime_s(&dst_start, &ds_time);
#else
        gmtime_r(&ds_time, &dst_start);
#endif

        int sundays = 0;
        while (sundays < 2)
        {
            if (dst_start.tm_wday == 0)
                sundays++;
            if (sundays < 2)
            {
                dst_start.tm_mday++;
                ds_time = timegm(&dst_start);
#ifdef _WIN32
                gmtime_s(&dst_start, &ds_time);
#else
                gmtime_r(&ds_time, &dst_start);
#endif
            }
        }

        // DST end: First Sunday of November at 2 AM EDT (6 AM UTC)
        struct tm dst_end = {};
        dst_end.tm_year = year - 1900;
        dst_end.tm_mon = 10; // November
        dst_end.tm_mday = 1;
        dst_end.tm_hour = 6; // 2 AM EDT = 6 AM UTC

        time_t de_time = timegm(&dst_end);
#ifdef _WIN32
        gmtime_s(&dst_end, &de_time);
#else
        gmtime_r(&de_time, &dst_end);
#endif

        while (dst_end.tm_wday != 0)
        {
            dst_end.tm_mday++;
            de_time = timegm(&dst_end);
#ifdef _WIN32
            gmtime_s(&dst_end, &de_time);
#else
            gmtime_r(&de_time, &dst_end);
#endif
        }

        return (timestamp >= ds_time && timestamp < de_time);
    }

    std::string convertToTimezone(int64_t timestamp, const std::string &timezone)
    {
        int offset = -5; // EST default
        if (timezone == "EDT")
            offset = -4;

        int dst_offset = isDst(timestamp) ? 1 : 0;
        int total_offset = offset + dst_offset;

        time_t adjusted = static_cast<time_t>(timestamp + total_offset * 3600);
        struct tm local_tm;
#ifdef _WIN32
        gmtime_s(&local_tm, &adjusted);
#else
        gmtime_r(&adjusted, &local_tm);
#endif

        std::string tz_label = dst_offset ? "EDT" : "EST";

        char buf[64];
        strftime(buf, sizeof(buf), "%Y-%m-%d %I:%M:%S %p", &local_tm);

        return std::string(buf) + " " + tz_label;
    }

} // namespace primebds::utils
