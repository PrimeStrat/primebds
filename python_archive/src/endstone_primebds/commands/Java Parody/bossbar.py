from endstone import Player, boss, ColorFormat
from endstone.command import CommandSender
from endstone_primebds.utils.command_util import create_command
from endstone_primebds.utils.target_selector_util import get_matching_actors
from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from endstone_primebds.primebds import PrimeBDS

boss_bar_cache = {}

command, permission = create_command(
    "bossbar",
    "Sets or clears a client-sided bossbar display!",
    [
        "/bossbar <player: player> (red|blue|green|yellow|pink|purple|rebecca_purple|white)<color: set_color> <percent: float> <title: message>",
        "/bossbar <player: player> clear"
    ],
    ["primebds.command.bossbar"]
)

def handler(self: "PrimeBDS", sender: CommandSender, args: list[str]) -> bool:
    global boss_bar_cache

    if not args:
        sender.send_message("Usage: /bossbar <player> <color> <percent> <title> OR /bossbar <player> clear")
        return False

    if len(args) == 2 and args[1].lower() == "clear":
        targets = get_matching_actors(self, args[0], sender)
        removed_count = 0
        for target in targets:
            if target in boss_bar_cache:
                bar = boss_bar_cache[target]
                bar.remove_player(target)
                del boss_bar_cache[target]
                removed_count += 1
        sender.send_message(f"Removed boss bar from {removed_count} player(s)")
        return True

    if len(args) < 4:
        sender.send_message("Usage: /bossbar <player> <color> <percent> <title>")
        return False
    
    color_name = args[1].upper()
    try:
        color = getattr(boss.BarColor, color_name)
    except AttributeError:
        sender.send_message(f"Invalid color '{args[1]}'. Using RED as default.")
        color = boss.BarColor.RED

    try:
        percent = float(args[2])
        percent = max(0.0, min(100.0, percent))
    except ValueError:
        sender.send_message(f"Invalid percent value '{args[2]}'. Must be a number 0-100.")
        return False

    title = " ".join(args[3:])
    style = boss.BarStyle.SOLID
    flags = []

    targets = get_matching_actors(self, args[0], sender)
    if not targets:
        sender.send_message(f"No matching players found for {args[0]}!")
        return False

    def remove_existing_bossbar(player: Player):
        if player in boss_bar_cache:
            existing_bar = boss_bar_cache[player]
            existing_bar.remove_player(player)
            del boss_bar_cache[player]

    for target in targets:
        remove_existing_bossbar(target)
        player_bar = self.server.create_boss_bar(f"{title}", color, style, flags)
        player_bar.progress = percent / 100
        player_bar.add_player(target)
        boss_bar_cache[target] = player_bar

    sender.send_message(
        f"§bBossbar Set For {len(targets)} player(s):\n"
        f"§7- §eColor: §r{args[1]}\n"
        f"§7- §epercent: §r{percent}%\n"
        f"§7- §eTitle: §r{title}\n"
    )
    return True
