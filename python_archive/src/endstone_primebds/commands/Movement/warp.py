from endstone import Player
from endstone.command import CommandSender
from endstone_primebds.utils.command_util import create_command
from endstone_primebds.utils.internal_permissions_util import get_permission_header
from endstone_primebds.utils.economy_utils import get_eco_link
from time import time

from endstone_primebds.utils.form_wrapper_util import (
    ActionFormData,
    ActionFormResponse,
)

from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from endstone_primebds.primebds import PrimeBDS

command, permission = create_command(
    "warp",
    "Warp to a warp location or browse warps!",
    [
        "/warp",
        "/warp [name: message]",
        "/warp (list)[warp_list: warp_list]"
    ],
    ["primebds.command.warp"]
)

warp_cooldowns: dict[str, float] = {}
warp_delays: dict[str, bool] = {}

def handler(self: "PrimeBDS", sender: CommandSender, args: list[str]) -> bool:
    if not isinstance(sender, Player):
        sender.send_error_message("This command can only be executed by a player")
        return False

    exempt_delay = sender.has_permission("primebds.exempt.warp.delays")
    exempt_cooldown = sender.has_permission("primebds.exempt.warp.cooldowns")

    current_time = time()
    last_used = warp_cooldowns.get(sender.id, 0)

    if not args or not args[0]:
        open_warp_menu(self, sender)
        return True

    sub = args[0].lower()
    warp_name = ""

    if sub == "list":
        warps = self.serverdb.get_all_warps(self.server)
        if not warps:
            sender.send_message("§cThere are no warps set")
            return True

        categories: dict[str, list[str]] = {}
        uncategorized: list[str] = []

        for name, warp in warps.items():
            cat = warp.get("category")
            display = warp.get("displayname") or name
            desc = warp.get("description")

            if desc:
                full_display = f"§b{display} §7- {desc}"
            else:
                full_display = f"§b{display}"

            if cat:
                categories.setdefault(cat, []).append(full_display)
            else:
                uncategorized.append(full_display)

        msg_lines: list[str] = []
        for cat, entries in categories.items():
            msg_lines.append(f"§6{cat}:")
            for line in entries:
                msg_lines.append(f"  §7- {line}")

        if uncategorized:
            for line in uncategorized:
                msg_lines.append(f"§7- {line}")

        sender.send_message("§aWarps:\n" + "\n".join(msg_lines))
        return True

    warp = self.serverdb.get_warp_fuzzy(sub, self.server)
    if not warp or not warp.get("pos"):
        warp_name = sub if not warp else warp.get("displayname") or sub
        sender.send_message(f"§cWarp §e{warp_name} §cdoes not exist")
        return True

    warp_name = warp.get("displayname") or warp["name"]

    warp_delay = 0 if exempt_delay else warp.get("delay", 0)
    warp_cooldown = 0 if exempt_cooldown else warp.get("cooldown", 0)

    if not exempt_cooldown and current_time - last_used < warp_cooldown:
        remaining = warp_cooldown - (current_time - last_used)
        sender.send_message(f"§cYou must wait {remaining:.1f}s before using this warp")
        return False

    start_pos = sender.location
    start_time = time()

    eco = get_eco_link(self)
    warp_cost = warp.get("cost", 0)
    if eco and warp_cost > 0:
        cost_form = ActionFormData()
        cost_form.title(f"Warp to {warp_name}")
        cost_form.body(f"This warp costs §e{warp_cost} coins.§r\n\nDo you want to continue?")
        cost_form.button("§aYes")
        cost_form.button("§cNo")

        def cost_submit(player: Player, cost_result: ActionFormResponse):
            if not cost_result or cost_result.selection != 0:
                player.send_message("§cWarp cancelled")
                return True

            bal = eco.api_get_player_money(player.name)
            if bal >= warp_cost:
                proceed_with_warp(self, player, warp, warp_name, warp_cost)
            else:
                player.send_message("§cWarp cancelled due to lack of funds")

        cost_form.show(sender).then(lambda player=sender, result=ActionFormResponse: cost_submit(player, result))
        return True 

    if warp_delay > 0:
        warp_delays[sender.id] = True
        sender.send_popup(f"§aWarping to §e{warp_name} §ain {warp_delay:.1f}s")

        def repeated_check():
            if sender.location.distance(start_pos) > 0.25:
                sender.send_message("§cTeleport cancelled because you moved!")
                warp_delays[sender.id] = False
                return True
            if time() - start_time >= warp_delay:
                sender.teleport(warp["pos"])
                sender.send_message(f"§aWarped to §e{warp_name}")
                warp_cooldowns[sender.id] = time()
                warp_delays[sender.id] = False
                return True
            remaining = max(0, warp_delay - (time() - start_time))
            sender.send_popup(f"§aWarping to §e{warp_name} §ain {remaining:.1f}s")
            return False

        self.server.scheduler.run_task(self, repeated_check, delay=0, period=20)
    else:
        sender.teleport(warp["pos"])
        sender.send_message(f"§aWarped to §e{warp_name}")
        warp_cooldowns[sender.id] = current_time

    return True

def open_warp_menu(self: "PrimeBDS", player: Player):
    warps = self.serverdb.get_all_warps(self.server)
    categories: dict[str, list[str]] = {}
    uncategorized: list[str] = []

    for name, warp in warps.items():
        display = warp.get("displayname") or name 
        cat = warp.get("category")
        if cat:
            categories.setdefault(cat, []).append(display)
        else:
            uncategorized.append(display)

    buttons = list(categories.keys())

    if uncategorized:
        buttons.append("Uncategorized")

    form = ActionFormData()
    form.title("Select Warp Category")
    for button in buttons:
        form.button(f"§b{button}")
    form.button("Close")

    def submit(player: Player, result: ActionFormResponse):
        if not result or result.selection is None or result.selection >= len(buttons):
            return True

        selected_cat = buttons[result.selection]
        warp_names = categories.get(selected_cat, []) + uncategorized

        if not warp_names:
            player.send_message("§cNo warps available")
            open_warp_menu(self, player)
            return True

        warp_form = ActionFormData()
        warp_form.title(f"Warps: {selected_cat}")
        for w in warp_names:
            warp_form.button(f"§r{w}")
        warp_form.button("Back")

        def warp_submit(player: Player, warp_result: ActionFormResponse):
            if not warp_result or warp_result.selection is None or warp_result.selection >= len(warp_names):
                open_warp_menu(self, player)
                return True

            selected_warp_name = warp_names[warp_result.selection]
            warp_data = self.serverdb.get_warp_fuzzy(selected_warp_name, self.server)
            if not warp_data or not warp_data.get("pos"):
                player.send_message(f"§cWarp §e{selected_warp_name} §cdoes not exist")
                open_warp_menu(self, player)
                return True
            
            warp_name = warp_data.get("displayname") or warp_data.get("name")
            
            eco = get_eco_link(self)
            warp_cost = warp_data.get("cost", 0)
            if eco and warp_cost > 0:
                cost_form = ActionFormData()
                cost_form.title(f"Warp to {warp_name}")
                cost_form.body(f"This warp costs §e{warp_cost} coins.§r\n\nDo you want to continue?")
                cost_form.button("§aYes")
                cost_form.button("§cNo")

                def cost_submit(player: Player, cost_result: ActionFormResponse):
                    if not cost_result or cost_result.selection != 0:
                        player.send_message("§cWarp cancelled")
                        return True

                    bal = eco.api_get_player_money(player.name)
                    if bal >= warp_cost:
                        proceed_with_warp(self, player, warp_data, warp_name, warp_cost)
                    else:
                        player.send_message("§cWarp cancelled due to lack of funds")

                cost_form.show(player).then(lambda player=player, result=ActionFormResponse: cost_submit(player, result))
                return True 

            proceed_with_warp(self, player, warp_data, warp_name)

        warp_form.show(player).then(lambda player=player, result=ActionFormResponse: warp_submit(player, result))

    form.show(player).then(lambda player=player, result=ActionFormResponse: submit(player, result))

def proceed_with_warp(self: "PrimeBDS", player: Player, warp_data: dict, warp_name: str, warp_cost: int = 0):
    if not isinstance(warp_data, dict):
        player.send_message(f"§cWarp data invalid for {warp_name}")
        return True
    
    warp_delay = warp_data.get("delay", 0)
    warp_cooldown = warp_data.get("cooldown", 0)
    now = time()

    exempt_delay = player.has_permission("primebds.exempt.warp.delays")
    exempt_cooldown = player.has_permission("primebds.exempt.warp.cooldowns")
    eco = get_eco_link(self)

    last_used = warp_cooldowns.get(player.id, 0)
    if not exempt_cooldown and now - last_used < warp_cooldown:
        remaining = warp_cooldown - (now - last_used)
        player.send_message(f"§cYou must wait {remaining:.1f}s before using this warp again")
        return True

    start_pos = player.location
    if warp_delay > 0 and not exempt_delay:
        warp_delays[player.id] = True
        player.send_popup(f"§7Warping to §e{warp_name} §7in {warp_delay:.1f}s")

        def repeated_check():
            if player.location.distance(start_pos) > 0.25:
                player.send_message("§cWarp cancelled because you moved!")
                warp_delays[player.id] = False
                return True
            if time() - now >= warp_delay:
                player.teleport(warp_data["pos"])
                player.send_message(f"§aWarped to §e{warp_name}")
                warp_cooldowns[player.id] = time()
                warp_delays[player.id] = False
                if eco:
                    name = get_permission_header(eco)
                    if warp_cost > 0:
                        if name == "umoney":
                            eco.api_change_player_money(player.name, -warp_cost)
                return True
            remaining = max(0, warp_delay - (time() - now))
            player.send_popup(f"§7Warping to §e{warp_name} §7in {remaining:.1f}s")
            return False

        self.server.scheduler.run_task(self, repeated_check, delay=0, period=20)
    else:
        player.teleport(warp_data["pos"])
        player.send_message(f"§aWarped to §e{warp_name}")
        warp_cooldowns[player.id] = now
        if eco:
            name = get_permission_header(eco)
            if warp_cost > 0:
                if name == "umoney":
                    eco.api_change_player_money(player.name, -warp_cost)
        