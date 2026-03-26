from datetime import datetime, timedelta

class TimezoneUtils:
    TIMEZONE_OFFSETS = {
        "EST": -5,
        "EDT": -4
    }

    @staticmethod
    def is_dst(timestamp):
        """Determines if a given timestamp falls within Daylight Saving Time (DST) for Eastern Time."""
        try:
            dt = datetime.utcfromtimestamp(timestamp)
        except (OSError, ValueError):
            return False

        year = dt.year

        # Second Sunday of March at 2 AM EST (7 AM UTC) - DST Start
        # Find March 1, then find the second Sunday
        dst_start = datetime(year, 3, 1, 7)  # 2 AM EST = 7 AM UTC
        sundays_found = 0
        while sundays_found < 2:
            if dst_start.weekday() == 6:
                sundays_found += 1
                if sundays_found == 2:
                    break
            dst_start += timedelta(days=1)

        # First Sunday of November at 2 AM EDT (6 AM UTC) - DST End
        dst_end = datetime(year, 11, 1, 6)  # 2 AM EDT = 6 AM UTC
        while dst_end.weekday() != 6:
            dst_end += timedelta(days=1)

        return dst_start <= dt < dst_end

    @staticmethod
    def convert_to_timezone(timestamp, timezone_name):
        """Converts a UTC timestamp to EST/EDT time with proper DST handling."""
        try:
            if timezone_name not in TimezoneUtils.TIMEZONE_OFFSETS:
                timezone_name = "EST"

            standard_offset = TimezoneUtils.TIMEZONE_OFFSETS[timezone_name]
            dst_offset = 1 if TimezoneUtils.is_dst(timestamp) else 0

            utc_time = datetime.utcfromtimestamp(timestamp)
            local_time = utc_time + timedelta(hours=standard_offset + dst_offset)
            timezone_label = "EDT" if dst_offset else "EST"

            return local_time.strftime('%Y-%m-%d %I:%M:%S %p') + f" {timezone_label}"

        except (OSError, ValueError, TypeError):
            return None
