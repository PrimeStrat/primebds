from endstone import Player
from endstone.inventory import ItemStack
from endstone.command import CommandSender
from endstone_primebds.utils.command_util import create_command
from endstone_primebds.utils.target_selector_util import get_matching_actors

try:
    from chest_form_api_endstone import ChestForm
    PACKET_SUPPORT = True
except Exception:
    PACKET_SUPPORT = False

from typing import TYPE_CHECKING
if TYPE_CHECKING:
    from endstone_primebds.primebds import PrimeBDS

# Register command
command, permission = create_command(
    "enderchest",
    "Allows you to view another player's ender chest!",
    ["/enderchest <player: player> (chat|chest)[echest_display: echest_display]"],
    ["primebds.command.enderchest"],
    "op",
    ["echest"]
)

def handler(self: "PrimeBDS", sender: CommandSender, args: list[str]) -> bool:
    if not isinstance(sender, Player):
        sender.send_message("§cThis command can only be executed by a player")
        return False

    if any("@a" in arg for arg in args):
        sender.send_message("§cYou cannot select all players for this command")
        return False
    
    display_type = "chest"
    if len(args) > 1 and args[1]:
        display_type = args[1].lower()

    if not PACKET_SUPPORT:
        display_type = "chat"
        sender.send_message("§cVisual UI is disabled due to missing API.")

    if not isinstance(sender, Player):
        display_type = "chat"

    targets = get_matching_actors(self, args[0], sender)

    if not targets:
        user = self.db.get_offline_user(args[0])
        if not user or not user.xuid:
            sender.send_message("§cNo matching player found")
            return False

        ender_inv = self.db.get_enderchest(user.xuid)
        if not ender_inv:
            sender.send_message(f"§cNo ender chest data found for {args[0]}")
            return False

        items = [normalize_item(entry) for entry in ender_inv]
        if display_type == "chat":
            sender.send_message(f"§6Ender Chest of §e{user.name}§6:\n{build_item_list(items)}")
        elif display_type == "chest":
            show_chest(self, sender, f"{user.name}'s Ender Chest", items, False)
        else:
            sender.send_message("§cInvalid display type. Use chat or chest.")
        return True

    for target in targets:
        items = [normalize_item(item, slot) for slot, item in enumerate(target.ender_chest.contents)]
        items = [i for i in items if i]

        if display_type == "chat":
            sender.send_message(f"§6Ender Chest of §e{target.name}§6:\n{build_item_list(items)}")
        elif display_type == "chest":
            show_chest(self, sender, f"{target.name}'s Ender Chest", items, False)
        else:
            sender.send_message("§cInvalid display type. Use chat or chest.")

    return True

def normalize_item(item, slot: int | None = None):
    def invalid_item(slot: int | None = None):
        return {
            "slot": slot,
            "type": "minecraft:barrier",
            "amount": 1,
            "data": 0,
            "display_name": "§cItem No Longer Exists",
            "lore": None,
            "enchants": None,
        }

    if not item:
        return None

    if isinstance(item, dict):
        item_id = item.get("type")
        amount = item.get("amount", 1)
        data = item.get("data", 0)

        try:
            _ = ItemStack(item_id, amount, data)
        except Exception:
            return invalid_item(item.get("slot"))

        return {
            "slot": item.get("slot"),
            "type": item_id,
            "amount": amount,
            "data": data,
            "display_name": item.get("display_name"),
            "lore": item.get("lore"),
            "enchants": item.get("enchants"),
        }

    else:
        item_id = getattr(getattr(item, "type", None), "id", None)
        amount = getattr(item, "amount", 1)
        data = getattr(item, "data", 0)

        try:
            _ = ItemStack(item_id, amount, data)
        except Exception:
            return invalid_item(slot)

        meta = getattr(item, "item_meta", None)
        return {
            "slot": slot,
            "type": item_id,
            "amount": amount,
            "data": data,
            "display_name": getattr(meta, "display_name", None) if meta else None,
            "lore": getattr(meta, "lore", None) if meta else None,
            "enchants": getattr(meta, "enchants", None) if meta else None,
        }

def build_item_list(items: list[dict]) -> str:
    return "\n".join(
        f"§7- §e{entry['type']} §7x{entry['amount']}"
        for entry in items if entry
    )

def show_chest(self, sender, title: str, items: list[dict], allow_armor: bool = False):
    chest = ChestForm(self, title, allow_armor)
    for entry in items:
        if not entry:
            continue
        slot = entry.get("slot")
        if slot is None:
            continue

        item_type = entry.get("type") or "minecraft:barrier"
        item_amount = entry.get("amount") or 1
        item_data = entry.get("data") or 0
        display_name = (
            entry.get("display_name")
            or ("§cInvalid Item" if item_type == "minecraft:barrier" else "")
        )

        chest.set_slot(
            slot,
            item_type,
            None,
            item_amount=item_amount,
            item_data=item_data,
            display_name=display_name,
            lore=entry.get("lore"),
            enchants=_format_enchants_for_chest(entry.get("enchants")),
        )
    chest.send_to(sender)


def _format_enchants_for_chest(e):
    """Normalize various enchant representations into a dict: {id: level}.

    ChestForm expects a dict mapping enchant id -> level.
    """
    if not e:
        return None

    if isinstance(e, dict):
        out = {}
        for k, v in e.items():
            try:
                out[str(k)] = int(v)
            except Exception:
                out[str(k)] = v
        return out if out else None

    if isinstance(e, (list, tuple)):
        out = {}
        for it in e:
            if isinstance(it, dict):
                key = it.get("id") or it.get("name") or it.get("key")
                lvl = it.get("level") or it.get("lvl") or it.get("value")
                if key:
                    try:
                        out[str(key)] = int(lvl) if lvl is not None else 1
                    except Exception:
                        out[str(key)] = lvl
            elif isinstance(it, (list, tuple)) and len(it) >= 2:
                try:
                    out[str(it[0])] = int(it[1])
                except Exception:
                    out[str(it[0])] = it[1]
        return out if out else None

    if isinstance(e, str):
        import json as _json, ast as _ast
        try:
            parsed = _json.loads(e)
            return _format_enchants_for_chest(parsed)
        except Exception:
            try:
                parsed = _ast.literal_eval(e)
                return _format_enchants_for_chest(parsed)
            except Exception:
                return None

    try:
        out = {}
        for k, v in e:
            try:
                out[str(k)] = int(v)
            except Exception:
                out[str(k)] = v
        return out if out else None
    except Exception:
        return None