from endstone.command import CommandSender
from endstone_primebds.utils.command_util import create_command

from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from endstone_primebds.primebds import PrimeBDS

# Register command
command, permission = create_command(
    "playtime",
    "Displays your total playtime or the server leaderboard.",
    ["/playtime [leaderboard: bool]"],
    ["primebds.command.playtime"]
)

# PLAYTIME COMMAND FUNCTIONALITY
def handler(self: "PrimeBDS", sender: CommandSender, args: list[str]) -> bool:
    player_name = sender.name
    player = self.server.get_player(player_name)

    if len(args) == 0 or args[0].lower() == 'false':

        # Fetch total playtime for the player
        total_playtime_seconds = self.sldb.get_total_playtime(player.xuid)
        total_playtime_minutes = total_playtime_seconds // 60
        total_playtime_hours = total_playtime_minutes // 60
        total_playtime_days = total_playtime_hours // 24
        total_playtime_hours %= 24
        total_playtime_minutes %= 60
        total_playtime_seconds %= 60

        leaderboard = self.sldb.get_all_playtimes()
        #print(leaderboard)
        leaderboard = sorted(leaderboard, key=lambda x: x['total_playtime'], reverse=True)

        player_rank = ""
        for index, entry in enumerate(leaderboard):
            if entry['name'] == player_name:
                player_rank = index + 1
                break

        if player_rank:
            rank_suffix = get_rank_suffix(player_rank)
        else:
            rank_suffix = "N/A"

        sender.send_message(
            f"§eYour Playtime: §r{total_playtime_days}d {total_playtime_hours}h {total_playtime_minutes}m {total_playtime_seconds}s §7§o({player_rank}{rank_suffix})§r")

        
    elif len(args) == 1 and args[0].lower() == 'true':
        
        leaderboard = self.sldb.get_all_playtimes()
        leaderboard = sorted(leaderboard, key=lambda x: x['total_playtime'], reverse=True)

        sender.send_message(f"§rTop 10 Playtimes on the Server:")

        # Show the top 10 players' playtimes
        for index, entry in enumerate(leaderboard[:10]):
            player_name = entry['name']
            total_playtime_seconds = entry['total_playtime']
            total_playtime_minutes = total_playtime_seconds // 60
            total_playtime_hours = total_playtime_minutes // 60
            total_playtime_days = total_playtime_hours // 24
            total_playtime_hours %= 24
            total_playtime_minutes %= 60
            total_playtime_seconds %= 60

            # Calculate the rank and its suffix
            rank = index + 1
            rank_suffix = get_rank_suffix(rank)

            sender.send_message(
                f"§a{rank}{rank_suffix}. §e{player_name} - §f{total_playtime_days}d {total_playtime_hours}h {total_playtime_minutes}m {total_playtime_seconds}s")

        
    else:
        sender.send_message(f"Usage: /playtime [leaderboard]")

    return True

def get_rank_suffix(rank) -> str:
    if 10 <= rank % 100 <= 20:
        suffix = 'th'
    else:
        suffix = {1: 'st', 2: 'nd', 3: 'rd'}.get(rank % 10, 'th')
    return suffix
