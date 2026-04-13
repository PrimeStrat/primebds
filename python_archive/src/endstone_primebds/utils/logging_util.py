import time
from datetime import datetime
from typing import TYPE_CHECKING
import requests
import re

from endstone_primebds.utils.config_util import load_config

if TYPE_CHECKING:
    from endstone_primebds.primebds import PrimeBDS

import threading

TOGGLE_PERMISSIONS = {
    "enabled_ms": "primebds.command.modspy",
    "enabled_as": "primebds.command.altspy",
}

def log(self: "PrimeBDS", message, type, toggles=None):
    if toggles is None:
        toggles = ["enabled_ms"]

    # Discord relay
    threading.Thread(target=discordRelay, args=(message, type)).start()

    players_to_notify = []
    
    for player in self.server.online_players:
        user = self.db.get_online_user(player.xuid)

        for toggle in toggles:
            perm = TOGGLE_PERMISSIONS.get(toggle)
            if perm and player.has_permission(perm):
                if getattr(user, toggle, 0): 
                    players_to_notify.append(player)
                    break 

    for player in players_to_notify:
        player.send_message(message)

    return False

def discordRelay(message, type):
    """Send message to Discord asynchronously without blocking."""
    message = re.sub(r'§.', '', message)  # Clean up formatting

    config = load_config()
    discord_logging = config["modules"]["discord_webhook"]

    webhook_url = get_webhook_url(type, discord_logging)
    if not webhook_url:
        return False  # No valid webhook found or enabled

    # Prepare the payload for Discord
    if discord_logging["embed_for_log"]["enabled"]:
        payload = {
            "embeds": [
                {
                    "title": discord_logging["embed_for_log"]["title"],
                    "description": message,
                    "color": discord_logging["embed_for_log"]["color"],
                    "footer": {
                        "text": f"Logged at {datetime.utcnow().strftime('%Y-%m-%d %H:%M:%S UTC')}"
                    }
                }
            ]
        }
    else:
        payload = {
            "content": f"{message}"
        }

    # Send Discord message asynchronously
    threading.Thread(target=send_discord_message, args=(webhook_url, payload)).start()
    return True

def get_webhook_url(type, discord_logging):
    """Helper function to get the appropriate webhook URL based on the message type."""
    if type == "cmd" and discord_logging["command_logs"]["enabled"]:
        return discord_logging["command_logs"]["webhook"]
    elif type == "mod" and discord_logging["moderation_logs"]["enabled"]:
        return discord_logging["moderation_logs"]["webhook"]
    elif type == "chat" and discord_logging["chat_logs"]["enabled"]:
        return discord_logging["chat_logs"]["webhook"]
    elif type == "connections" and discord_logging["connection_logs"]["enabled"]:
        return discord_logging["connection_logs"]["webhook"]
    return None

MAX_RETRIES = 15  # Max retries in case of rate limits
INITIAL_BACKOFF = 1  # Start with 1 second
def send_discord_message(webhook_url, payload):
    """Send HTTP request to Discord webhook with exponential backoff."""
    retries = 0
    backoff = INITIAL_BACKOFF 

    while retries < MAX_RETRIES:
        response = None
        try:
            response = requests.post(webhook_url, json=payload, timeout=10)
            response.raise_for_status()
            return True
        except requests.exceptions.RequestException as e:
            if response is not None and response.status_code == 429:
                retries += 1
                wait_time = backoff * (2 ** retries)
                print(f"[primebds - Discord Log] Rate limit exceeded. Retrying in {wait_time}s...")
                time.sleep(wait_time)
            else:
                print(f"Failed to send Discord message: {e}")
                return False

    print("Max retries reached. Failed to send message.")
    return False
