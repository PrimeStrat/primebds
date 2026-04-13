from typing import TYPE_CHECKING, Callable, Optional

if TYPE_CHECKING:
    from endstone_primebds.primebds import PrimeBDS
    from endstone import Player

class IntervalManager:
    """Manages scheduled periodic checks for the plugin."""

    def __init__(self, plugin: "PrimeBDS", tick_interval: int = 20):
        self.plugin = plugin
        self.tick_interval = tick_interval
        self._task_id: Optional[int] = None
        self._check_functions: list[Callable[["PrimeBDS"], None]] = []
        self._player_checks: dict[str, Callable[["PrimeBDS", "Player"], None]] = {}

    def add_check(self, func: Callable[["PrimeBDS"], None]):
        """Add a function to be called every interval."""
        if func not in self._check_functions:
            self._check_functions.append(func)

    def remove_check(self, func: Callable[["PrimeBDS"], None]):
        """Remove a previously added check function."""
        if func in self._check_functions:
            self._check_functions.remove(func)

    def add_player_check(self, player_name: str, func: Callable[["PrimeBDS", "Player"], None]):
        """Register a repeating function for a specific player."""
        self._player_checks[player_name] = func

    def remove_player_check(self, player_name: str):
        """Remove a player-specific check."""
        self._player_checks.pop(player_name, None)

    def clear_all_player_checks(self):
        """Remove all player-specific checks."""
        self._player_checks.clear()

    def start(self):
        """Start the repeating task."""
        if self._task_id is not None:
            return  # Already running
        task = self.plugin.server.scheduler.run_task(
            self.plugin, lambda: self._run_checks(), 0, self.tick_interval
        )
        self._task_id = task.task_id

    def stop(self):
        """Stop the repeating task."""
        if self._task_id is not None:
            self.plugin.server.scheduler.cancel_task(self._task_id)
            self._task_id = None

    def _run_checks(self):
        """Run all global and player-specific checks."""
        # Run global checks
        for func in self._check_functions:
            try:
                func(self.plugin)
            except Exception as e:
                print(f"Error in interval check {func.__name__}: {e}")

        # Run player-specific checks
        for player_name, func in list(self._player_checks.items()):
            player = self.plugin.server.get_player(player_name)
            if not player:
                # Remove disconnected players
                self.remove_player_check(player_name)
                continue
            try:
                func(self.plugin, player)
            except Exception as e:
                # Remove if error occurs
                self.remove_player_check(player_name)
                print(f"Error in player interval {player_name}: {e}")