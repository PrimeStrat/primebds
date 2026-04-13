def create_command(command_name: str, description: str, usages: list, permissions: list, default: str = "op", aliases: list = None):
    # Create the command dictionary with all its details
    command = {
        command_name: {
            "description": description,
            "usages": usages,
            "permissions": permissions,
            "aliases": aliases if aliases else []
        }
    }

    # Endstone permission
    permission = {
        permissions[0]: {
            "description": f"Allows use of the {command_name} command",
            "default": default
        }
    }

    return command, permission
