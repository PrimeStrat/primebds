from endstone.command import CommandSender
from endstone_primebds.utils.command_util import create_command
from endstone_primebds.utils.config_util import load_rules

from typing import TYPE_CHECKING
if TYPE_CHECKING:
    from endstone_primebds.primebds import PrimeBDS

# Register command
command, permission = create_command(
    "rules",
    "See this server's rules!",
    ["/rules"],
    ["primebds.command.rules"]
)

def handler(self: "PrimeBDS", sender: CommandSender, args: list[str]) -> bool:
    rules = load_rules()
    if isinstance(rules, list):
        formatted = "\n".join(str(rule) for rule in rules)
    else:
        formatted = str(rules)

    sender.send_message(formatted)
    return True
