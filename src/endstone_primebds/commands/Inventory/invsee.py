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

command, permission = create_command(
    "invsee",
    "Allows you to view another player's inventory!",
    ["/invsee <player: player> (chat|chest)[invsee_display: invsee_display]"],
    ["primebds.command.invsee"]
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
        sender.send_message("§cVisual UI is disabled due to missing API.")
        display_type = "chat"

    targets = get_matching_actors(self, args[0], sender)

    if not targets:
        user = self.db.get_offline_user(args[0])
        if not user or not user.xuid:
            sender.send_message("§cNo matching player found")
            return False

        combined_inventory = self.db.get_inventory(user.xuid)
        if not combined_inventory:
            sender.send_message("§cNo inventory found for that player")
            return False

        items = [normalize_item(entry) for entry in combined_inventory]
        if display_type == "chat":
            sender.send_message(f"§6Inventory of §e{user.name}§6:\n{build_item_list(items)}")
        elif display_type == "chest":
            show_chest(self, sender, f"{user.name}'s Inventory", items, True)
        else:
            sender.send_message("§cInvalid display type")
        return True

    for target in targets:
        inv_items = [normalize_item(item, slot=slot) for slot, item in enumerate(target.inventory.contents)]
        armor_items = [
            normalize_item(target.inventory.helmet, slot_type="helmet"),
            normalize_item(target.inventory.chestplate, slot_type="chestplate"),
            normalize_item(target.inventory.leggings, slot_type="leggings"),
            normalize_item(target.inventory.boots, slot_type="boots"),
            normalize_item(target.inventory.item_in_off_hand, slot_type="offhand")
        ]

        items = [i for i in inv_items + armor_items if i]

        if display_type == "chat":
            sender.send_message(f"§6Inventory of §e{target.name}§6:\n{build_item_list(items)}")
        elif display_type == "chest":
            show_chest(self, sender, f"{target.name}'s Inventory", items, True)
        else:
            sender.send_message("§cInvalid display type")

    return True

def normalize_item(item, slot: int | None = None, slot_type: str | None = None):
    def invalid_item(slot: int | None = None, slot_type: str | None = None):
        return {}

    if not item:
        return None

    if isinstance(item, dict):
        item_id = item.get("type")
        amount = item.get("amount", 1)
        data = item.get("data", 0)

        try:
            _ = ItemStack(item_id, amount, data)
        except Exception:
            return invalid_item(item.get("slot"), item.get("slot_type"))

        return {
            "slot": item.get("slot"),
            "slot_type": item.get("slot_type"),
            "type": item_id,
            "amount": amount,
            "data": data,
            "display_name": item.get("display_name"),
            "lore": item.get("lore"),
            "enchants": _normalize_enchants(item.get("enchants")),
        }

    else:
        item_id = getattr(getattr(item, "type", None), "id", None)
        amount = getattr(item, "amount", 1)
        data = getattr(item, "data", 0)

        try:
            _ = ItemStack(item_id, amount, data)
        except Exception:
            return invalid_item(slot, slot_type)

        meta = getattr(item, "item_meta", None)
        return {
            "slot": slot,
            "slot_type": slot_type,
            "type": item_id,
            "amount": amount,
            "data": data,
            "display_name": getattr(meta, "display_name", None) if meta else None,
            "lore": getattr(meta, "lore", None) if meta else None,
            "enchants": _normalize_enchants(getattr(meta, "enchants", None)) if meta else None,
        }


def _enchant_key_to_str(k) -> str:
    try:
        if k is None:
            return "None"
        if isinstance(k, str):
            return k
        return getattr(k, "name", None) or getattr(k, "key", None) or str(k)
    except Exception:
        return str(k)

def _normalize_enchants(src):
    """Return a mapping of enchant-name -> level or None if unknown.

    Accepts dicts, lists, or string-encoded dicts (JSON/SNBT-like)."""
    if src is None:
        return None

    if isinstance(src, dict):
        out = {}
        for k, v in src.items():
            key = _enchant_key_to_str(k)
            try:
                out[key] = int(v)
            except Exception:
                out[key] = v
        return out

    if isinstance(src, (list, tuple)):
        out = {}
        for e in src:
            if isinstance(e, dict):
                key = e.get("id") or e.get("name") or e.get("enchant") or e.get("key")
                lvl = e.get("level") or e.get("lvl") or e.get("value")
                if key:
                    try:
                        out[str(key)] = int(lvl) if lvl is not None else 1
                    except Exception:
                        out[str(key)] = lvl
            elif isinstance(e, (list, tuple)) and len(e) >= 2:
                try:
                    out[str(e[0])] = int(e[1])
                except Exception:
                    out[str(e[0])] = e[1]
        return out

    if isinstance(src, str):
        import json as _json
        import ast as _ast
        try:
            parsed = _json.loads(src)
            return _normalize_enchants(parsed)
        except Exception:
            try:
                parsed = _ast.literal_eval(src)
                return _normalize_enchants(parsed)
            except Exception:
                return None

    try:
        out = {}
        for k, v in src:
            out[_enchant_key_to_str(k)] = int(v)
        return out
    except Exception:
        return None

def build_item_list(items: list[dict]) -> str:
    lines = []
    for entry in items:
        if not entry:
            continue
        line = f"§7- §e{entry['type']} §7x{entry['amount']}"
        if entry.get("slot_type") in ("helmet","chestplate","leggings","boots","offhand","mainhand"):
            line += " §7(equipped)"
        lines.append(line)
    return "\n".join(lines)

def slot_mapping(entry: dict) -> int | None:
    slot_type = entry.get("slot_type")
    if isinstance(entry.get("slot"), int):
        raw_slot = entry["slot"]
        if 9 <= raw_slot <= 35:
            return raw_slot - 9
        elif 0 <= raw_slot <= 8:
            return 36 + raw_slot
    if slot_type in ("helmet","chestplate","leggings","boots","offhand","mainhand"):
        equip_map = {
            "helmet": 50,
            "chestplate": 51,
            "leggings": 52,
            "boots": 53,
            "offhand": 49,
            "mainhand": 48
        }
        return equip_map.get(slot_type)
    return None

def show_chest(self, sender, title: str, items: list[dict], allow_armor: bool):
    chest = ChestForm(self, title, allow_armor)
    for entry in items:
        if not entry:
            continue

        chest_slot = slot_mapping(entry)
        if chest_slot is None:
            continue
        
        item_type = entry.get("type") or "minecraft:barrier"
        item_amount = entry.get("amount") or 1
        item_data = entry.get("data") or 0
        display_name = (
            entry.get("display_name")
            or ("§cInvalid Item" if item_type == "minecraft:barrier" else "")
        )

        chest.set_slot(
            chest_slot,
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

    # If already a dict mapping
    if isinstance(e, dict):
        out = {}
        for k, v in e.items():
            try:
                out[str(k)] = int(v)
            except Exception:
                out[str(k)] = v
        return out if out else None

    # If list of pairs or dicts
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

    # If string, try JSON then literal_eval
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

    # Last resort: try iterable of pairs
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

