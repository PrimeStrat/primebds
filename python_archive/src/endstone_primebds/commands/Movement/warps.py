from endstone import Player
from endstone.command import CommandSender
from endstone_primebds.utils.command_util import create_command

from endstone_primebds.utils.form_wrapper_util import (
    ActionFormData,
    ModalFormData,
    ActionFormResponse,
    ModalFormResponse
)

from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from endstone_primebds.primebds import PrimeBDS

command, permission = create_command(
    "warps",
    "Manage server warps!",
    [
        "/warps",
        "/warps (list)[warps_list: warps_list]",
        "/warps (create)[warp_create: warp_create] <name: string> [displayname: string] [category: string] [description: string] [delay: int] [cooldown: int] [cost: int]",
        "/warps (delete)[warp_delete: warp_delete] <name: message>",
        "/warps (addalias)[warp_alias_add: warp_alias_add] <name: string> <alias: message>",
        "/warps (removealias)[warp_alias_remove: warp_alias_remove] <name: string> <alias: message>",
        "/warps (setcost)[warp_setcost: warp_setcost] <name: str> <cost: float>",
        "/warps (setdescription)[warp_setdescription: warp_setdescription] <name: str> <description: message>",
        "/warps (setcategory)[warp_setcategory: warp_setcategory] <name: str> <category: message>",
        "/warps (setdisplayname)[warp_setdisplayname: warp_setdisplayname] <name: str> <displayname: message>",
        "/warps (setdelay)[warp_setdelay: warp_setdelay] <name: str> <delay: float>",
        "/warps (setcooldown)[warp_setcooldown: warp_setcooldown] <name: str> <cooldown: float>"
    ],
    ["primebds.command.warps"]
)

warp_cooldowns: dict[str, float] = {}
warp_delays: dict[str, bool] = {}

def handler(self: "PrimeBDS", sender: CommandSender, args: list[str]) -> bool:
    if not isinstance(sender, Player):
        sender.send_error_message("This command can only be executed by a player")
        return False
    
    if not args:
        open_warps_menu(self, sender)
        return True

    sub = args[0].lower()

    if sub == "list":
        warps = self.serverdb.get_all_warps(self.server)
        if not warps:
            sender.send_message("§cThere are no warps set")
            return True

        categories: dict[str, list[str]] = {}
        uncategorized: list[str] = []

        for name, warp in warps.items():
            cat = warp.get("category")
            id = name
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
            msg_lines.append("§6Uncategorized:")
            for line in uncategorized:
                msg_lines.append(f"§7- {line}")

        sender.send_message("§aWarps:\n" + "\n".join(msg_lines))
        return True

    if sub == "create" and len(args) >= 2:
        name = args[1]

        displayname = args[2] if len(args) >= 3 else None
        category = args[3] if len(args) >= 4 else None
        description = args[4] if len(args) >= 5 else None

        try:
            cost = float(args[5]) if len(args) >= 6 else 0.0
        except ValueError:
            cost = 0.0

        try:
            cooldown = int(args[6]) if len(args) >= 7 else 0
        except ValueError:
            cooldown = 0

        try:
            delay = int(args[7]) if len(args) >= 8 else 0
        except ValueError:
            delay = 0

        loc = sender.location

        created = self.serverdb.create_warp(
            name=name,
            location=loc,
            displayname=displayname,
            category=category,
            description=description,
            cost=cost,
            cooldown=cooldown,
            delay=delay
        )

        if created:
            sender.send_message(f"§aWarp §e{name} §aset at your current location")
        else:
            sender.send_message(f"§cWarp §e{name} §calready exists")

        return True

    if sub == "delete" and len(args) >= 2:
        name = args[1]
        if self.serverdb.delete_warp(name):
            sender.send_message(f"§aWarp §e{name} §adeleted")
        else:
            sender.send_message(f"§cWarp §e{name} §cdoes not exist")
        return True

    if sub.startswith("set") and len(args) >= 3:
        name = args[1]
        warp = self.serverdb.get_warp_fuzzy(name, self.server)
        if not warp:
            sender.send_message(f"§cWarp §e{name} §cdoes not exist")
            return True

        if sub == "setcost":
            try:
                cost = float(args[2])
                self.serverdb.update_warp_property(name, "cost", cost)
                sender.send_message(f"§aWarp §e{name} §acost updated to §e{cost}")
            except ValueError:
                sender.send_message("§cInvalid cost value")
            return True

        if sub == "setdescription":
            description = " ".join(args[2:])
            self.serverdb.update_warp_property(name, "description", description)
            sender.send_message(f"§aWarp §e{name} §adescription updated")
            return True

        if sub == "setcategory":
            category = args[2]
            self.serverdb.update_warp_property(name, "category", category)
            sender.send_message(f"§aWarp §e{name} §acategory updated to §e{category}")
            return True

        if sub == "setdisplayname":
            displayname = " ".join(args[2:])
            self.serverdb.update_warp_property(name, "displayname", displayname)
            sender.send_message(f"§aWarp §e{name} §adisplay name updated")
            return True

        if sub == "setdelay":
            try:
                delay = float(args[2])
                self.serverdb.update_warp_property(name, "delay", delay)
                sender.send_message(f"§aWarp §e{name} §adelay updated to §e{delay}s")
            except ValueError:
                sender.send_message("§cInvalid delay value")
            return True

        if sub == "setcooldown":
            try:
                cooldown = float(args[2])
                self.serverdb.update_warp_property(name, "cooldown", cooldown)
                sender.send_message(f"§aWarp §e{name} §acooldown updated to §e{cooldown}s")
            except ValueError:
                sender.send_message("§cInvalid cooldown value")
            return True

    if sub == "addalias":
        if len(args) < 3:
            sender.send_message("§cUsage: /warps addalias <warp> <alias>")
            return True

        warp_name = args[1]
        alias = " ".join(args[2:])

        warp = self.serverdb.get_warp_fuzzy(warp_name, self.server)
        if not warp:
            sender.send_message(f"§cWarp §e{warp_name} §cnot found")
            return True

        if self.serverdb.add_alias(warp["name"], alias):
            sender.send_message(f"§aAlias §e{alias} §aadded to warp §e{warp['name']}")
        else:
            sender.send_message("§cCould not add alias")
        return True


    if sub == "removealias":
        if len(args) < 3:
            sender.send_message("§cUsage: /warps removealias <warp> <alias>")
            return True

        warp_name = args[1]
        alias = " ".join(args[2:])

        warp = self.serverdb.get_warp_fuzzy(warp_name, self.server)
        if not warp:
            sender.send_message(f"§cWarp §e{warp_name} §cnot found")
            return True

        if self.serverdb.remove_alias(warp["name"], alias):
            sender.send_message(f"§aAlias §e{alias} §aremoved from warp §e{warp['name']}")
        else:
            sender.send_message("§cCould not remove alias")
        return True

    sender.send_message("§cInvalid usage or missing arguments for /warps")
    return False

def open_warps_menu(self: "PrimeBDS", player):
    warps = self.serverdb.get_all_warps(self.server)

    if not warps:
        player.send_message("§cNo warps exist")
        return

    warp_list = sorted(warps.keys(), key=lambda x: x.lower())

    form = ActionFormData()
    form.title("Warp Editor")
    form.body("Select a warp to modify:")

    for name in warp_list:
        w = warps[name]
        label = w.get("displayname") or w["name"]
        form.button(f"§e{label}")

    form.button("Close")

    def submit(player: Player, res: ActionFormResponse):
        if not res or res.canceled or res.selection >= len(warp_list):
            return True

        warp_name = warp_list[res.selection]
        warp_data = warps[warp_name]

        open_warp_edit_menu(self, player, warp_data)

    form.show(player).then(lambda player=player, result=ActionFormResponse: submit(player, result))

def open_warp_edit_menu(self: "PrimeBDS", player, warp: dict):
    form = ModalFormData()
    form.title(f"Edit Warp: {warp['name']}")

    form.text_field("Display Name", warp.get("displayname") or "Optional", warp.get("displayname") or "")
    form.text_field("Category", warp.get("category") or "Optional", warp.get("category") or "")
    form.text_field("Description", warp.get("description") or "Optional", warp.get("description") or "")
    form.text_field("Aliases (comma separated)", (", ".join(warp.get("aliases"))) or "home, base, ext.", ", ".join(warp.get("aliases") or []))

    form.text_field("Cost", str(warp.get("cost", 0)), str(warp.get("cost", 0)))
    form.text_field("Cooldown", str(warp.get("cooldown", 0)), str(warp.get("cooldown", 0)))
    form.text_field("Delay", str(warp.get("delay", 0)), str(warp.get("delay", 0)))

    def submit(player: Player, res: ModalFormResponse):
        if not res or res.canceled:
            return True

        displayname, category, description, alias_text, cost_str, cooldown_str, delay_str = res.formValues
        aliases = [a.strip() for a in alias_text.split(",") if a.strip()]

        try:
            cost = float(cost_str)
            cooldown = int(cooldown_str)
            delay = int(delay_str)
        except ValueError:
            player.send_message("§cInvalid numeric input.")
            return True

        self.serverdb.update_warp_property(warp["name"], "displayname", displayname)
        self.serverdb.update_warp_property(warp["name"], "category", category)
        self.serverdb.update_warp_property(warp["name"], "description", description)
        self.serverdb.update_warp_property(warp["name"], "aliases", aliases)
        self.serverdb.update_warp_property(warp["name"], "cost", cost)
        self.serverdb.update_warp_property(warp["name"], "cooldown", cooldown)
        self.serverdb.update_warp_property(warp["name"], "delay", delay)

        player.send_message(f"§aWarp §e{warp['name']} §aupdated successfully")

    form.show(player).then(lambda player=player, result=ModalFormResponse: submit(player, result))