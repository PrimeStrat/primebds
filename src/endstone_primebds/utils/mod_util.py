from datetime import datetime, timedelta
from endstone import ColorFormat, Player
from endstone.inventory import ItemStack

from typing import TYPE_CHECKING
if TYPE_CHECKING:
    from endstone_primebds.primebds import PrimeBDS


def ban_message(server, expiration, reason) -> str:
    return (
        f"{ColorFormat.RED}You were banned from {ColorFormat.GOLD}{server}\n"
        f"{ColorFormat.RED}Punishment Expires: {ColorFormat.GOLD}{expiration}\n"
        f"{ColorFormat.RED}Punishment Reason: {ColorFormat.GOLD}{reason}"
    )

def format_time_remaining(expiration, is_not_ban = False) -> str:
    expiration_datetime = datetime.fromtimestamp(expiration)
    now = datetime.now()
    remaining = expiration_datetime - now

    if remaining.total_seconds() <= 0:
        return "Expired"

    if remaining.total_seconds() > 50 * 31536000:

        if is_not_ban:
            return "Never"

        return "Never - Permanent Ban"

    total_seconds = int(remaining.total_seconds())

    years, rem = divmod(total_seconds, 31536000)  # 365 days
    months, rem = divmod(rem, 2592000)  # 30 days
    weeks, rem = divmod(rem, 604800)  # 7 days
    days, rem = divmod(rem, 86400)  # 1 day
    hours, rem = divmod(rem, 3600)  # 1 hour
    minutes, seconds = divmod(rem, 60)  # 1 minute

    time_parts = []
    if years: time_parts.append(f"{years} year{'s' if years > 1 else ''}")
    if months: time_parts.append(f"{months} month{'s' if months > 1 else ''}")
    if weeks: time_parts.append(f"{weeks} week{'s' if weeks > 1 else ''}")
    if days: time_parts.append(f"{days} day{'s' if days > 1 else ''}")
    if hours: time_parts.append(f"{hours} hour{'s' if hours > 1 else ''}")
    if minutes: time_parts.append(f"{minutes} minute{'s' if minutes > 1 else ''}")
    if seconds: time_parts.append(f"{seconds} second{'s' if seconds > 1 else ''}")

    if not time_parts:
        return "0 seconds"
    
    return ", ".join(time_parts)

MAX_DURATION_SECONDS = 60 * 60 * 24 * 365 * 10 
def safe_duration(seconds: int) -> timedelta:
    """
    Clamp large duration seconds to a max limit to avoid OverflowError in timedelta.
    """
    if seconds < 0:
        seconds = 0
    if seconds > MAX_DURATION_SECONDS:
        seconds = MAX_DURATION_SECONDS
    return timedelta(seconds=seconds)