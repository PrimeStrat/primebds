from endstone.command import CommandSender
from endstone_primebds.utils.command_util import create_command
from endstone_primebds.utils.config_util import load_rules, save_rules

from typing import TYPE_CHECKING
if TYPE_CHECKING:
    from endstone_primebds.primebds import PrimeBDS

# Register command
command, permission = create_command(
    "setrules",
    "Add, remove, or modify rules displayed by /discord.",
    [
        "/setrules (add)<add_rule: add_rule> <text: message>",
        "/setrules (edit)<edit_rule: edit_rule> <index: int> <text: message>",
        "/setrules (delete)<delete_rule: delete_rule> <index: int>",
        "/setrules (insert)<insert_rule: insert_rule> <index: int> <text: message>",
        "/setrules (list)<list_rules: list_rules>"
    ],
    ["primebds.command.setrules"],
    "op"
)

def handler(self: "PrimeBDS", sender: CommandSender, args: list[str]) -> bool:
    action = args[0].lower()
    rules = load_rules()

    if not isinstance(rules, list):
        rules = []

    if action == "list":
        if not rules:
            sender.send_message("§7No rules currently set.")
            return True

        sender.send_message("§aCurrent Rules:")
        for i, rule in enumerate(rules, start=1):
            sender.send_message(f"§7[{i}] §r{rule}")
        return True

    elif action == "add":
        new_rule = " ".join(args[1:])
        rules.append(new_rule)
        save_rules(rules)

        sender.send_message(f"§aAdded new rule (§e#{len(rules)}§a): {new_rule}")
        return True

    elif action == "edit":
        try:
            index = int(args[1]) - 1
            if index < 0 or index >= len(rules):
                raise IndexError
        except (ValueError, IndexError):
            sender.send_message("§cInvalid rule index.")
            return True

        new_text = " ".join(args[2:])
        old_text = rules[index]
        rules[index] = new_text
        save_rules(rules)

        sender.send_message(f"§aUpdated rule §e#{index + 1}§a:\n§7Old: {old_text}\n§aNew: {new_text}")
        return True

    elif action == "delete":
        try:
            index = int(args[1]) - 1
            if index < 0 or index >= len(rules):
                raise IndexError
        except (ValueError, IndexError):
            sender.send_message("§cInvalid rule index.")
            return True

        removed = rules.pop(index)
        save_rules(rules)

        sender.send_message(f"§cDeleted rule §e#{index + 1}§c: {removed}")
        return True
    
    elif action == "insert":
        try:
            index = int(args[1]) - 1
            if index < 0 or index > len(rules):
                raise IndexError
        except (ValueError, IndexError):
            sender.send_message("§cInvalid index.")
            return True

        new_rule = " ".join(args[2:])
        rules.insert(index, new_rule)
        save_rules(rules)

        sender.send_message(f"§aInserted new rule at position §e#{index + 1}§a: {new_rule}")
        return True

    else:
        sender.send_message("§cUnknown action. Use: add, edit, delete, or list.")
        return True
